#include <fstream>
#include <algorithm>
#include <omp.h>
#include <iomanip>
#include "comm_analysis.h"
#include "record/record_meta.h"
#include "instrument/backtrace.h"

inline std::string encode_backtrace(backtrace_context_t ctx, const BacktraceTree &tree) {
    std::stringbuf sb;
    while (ctx != BACKTRACE_ROOT_NODE) {
        const auto ip = tree.backtrace_context_ip(ctx);
        sb.sputn(reinterpret_cast<const char *>(&ip), sizeof(uint64_t));
        ctx = tree.get_parent(ctx);
    }
    return sb.str();
}

double get_comm_duration(record_t* r, uint64_t mhz) {
	if(r->timestamps.exit < r->timestamps.enter || r->timestamps.exit > 10 * r->timestamps.enter) return 0;
    return tsc_duration_us(r->timestamps.exit - r->timestamps.enter, mhz);
}

void print_backtrace(std::ofstream &f_out, BacktraceCollection& _backtraces, const CommunicationAnalyzer::CommAnalyzeItem* item) {
	std::vector<const char*> bt_vec;
	_backtraces[item->rank]->backtrace_get_context_string_vec(item->data->ctxt, 20, bt_vec);

	for(auto c : bt_vec) {
		f_out << c << ";";
	}
	f_out << std::endl;
}

void CommunicationAnalyzer::get_comm_world_from_meta(RankMetaCollection& metas, int rank) {
	MetaDataMap::MetaValue_t *t;
	const std::unordered_map<std::string, MetaDataMap *> & metaMap = metas[rank]->getMetaMap();
	metaMap.at("MPI_COMM_WORLD")->get("MPI_COMM_WORLD", &t);
	_comm2name[(uint64_t)t->i64] = "MPI_COMM_WORLD";
}

void CommunicationAnalyzer::get_comm_world_from_meta(RankMetaCollection& metas, int rank, std::unordered_map<uint64_t, std::string> &comm2name) {
	MetaDataMap::MetaValue_t *t;
	const std::unordered_map<std::string, MetaDataMap *> & metaMap = metas[rank]->getMetaMap();
	metaMap.at("MPI_COMM_WORLD")->get("MPI_COMM_WORLD", &t);
	comm2name[(uint64_t)t->i64] = "MPI_COMM_WORLD";
}

// for comm_dup
void CommunicationAnalyzer::add_Comm_to_map(int rank, uint64_t old_comm, uint64_t new_comm) {
	int pos = 0;
	if(_commrank2dup_count.find(_comm2name[old_comm] + std::to_string(rank)) == _commrank2dup_count.end()) _commrank2dup_count[_comm2name[old_comm] + std::to_string(rank)] = 0;
	// Each rank corresponding to a communication sub needs to record the number of the new communication sub produced after its own named dup and split.
	_ori_comm2dup_count[_comm2name[old_comm]] = _commrank2dup_count[_comm2name[old_comm] + std::to_string(rank)]++;
	if((pos = _comm2name[old_comm].find_first_of("dup")) == std::string::npos) {
		_comm2name[new_comm] = _comm2name[old_comm] + "dup" + std::to_string(_ori_comm2dup_count[_comm2name[old_comm]]);
	}
	else _comm2name[new_comm] = _comm2name[old_comm].substr(0, pos) + std::to_string(_ori_comm2dup_count[_comm2name[old_comm]]);
}

// for comm_split
void CommunicationAnalyzer::add_Comm_to_map(int rank, uint64_t old_comm, int color, uint64_t new_comm) {
	if(_commrank2split_count.find(_comm2name[old_comm] + std::to_string(rank)) == _commrank2split_count.end()) _commrank2split_count[_comm2name[old_comm] + std::to_string(rank)] = 0;
	_ori_comm2split_count[_comm2name[old_comm]] = _commrank2split_count[_comm2name[old_comm] + std::to_string(rank)]++;
	if(_old_comm_name_to_color2new_name.find(_comm2name[old_comm]) == _old_comm_name_to_color2new_name.end()) {
		_comm2name[new_comm] = _comm2name[old_comm] + "split" + std::to_string(color) + std::to_string(_ori_comm2split_count[_comm2name[old_comm]]);
		_old_comm_name_to_color2new_name[_comm2name[old_comm]][color] = _comm2name[new_comm];
	} else {
		if(_old_comm_name_to_color2new_name[_comm2name[old_comm]].find(color) == _old_comm_name_to_color2new_name[_comm2name[old_comm]].end()) {
			_comm2name[new_comm] = _comm2name[old_comm] + "split" + std::to_string(color) + std::to_string(_ori_comm2split_count[_comm2name[old_comm]]);
			_old_comm_name_to_color2new_name[_comm2name[old_comm]][color] = _comm2name[new_comm];
		} else {
			_comm2name[new_comm] = _old_comm_name_to_color2new_name[_comm2name[old_comm]][color];
		}
	}
}

uint64_t get_comm_volume(record_t* r) {
    switch(r->MsgType) {
	case event_MPI_Send:
	case event_MPI_Recv: {
		record_comm_t *rd = (record_comm_t *) r;
		return rd->typesize * rd->count;
	}
	case event_MPI_Isend:
	case event_MPI_Irecv: {
		record_comm_async_t *rd = (record_comm_async_t *) r;
		return rd->typesize * rd->count;
	}
	case event_MPI_Alltoall:
	case event_MPI_Alltoallv: {
		record_all2all_t *rd = (record_all2all_t *) r;
		return rd->typesize * (rd->sendcnt+rd->recvcnt);
	}
	case event_MPI_Allreduce: {
		record_allreduce_t *rd = (record_allreduce_t *) r;
		return rd->typesize * rd->count;
	}
	case event_MPI_Reduce: {
		record_reduce_t *rd = (record_reduce_t *) r;
		return rd->typesize * rd->count;
	}
	case event_MPI_Bcast: {
		record_bcast_t *rd = (record_bcast_t *) r;
		return rd->typesize * rd->count;
	}
    }
    return 0;
}

bool record_is_comm(record_t* r) {
    return RecordHelper::is_event(r, event_MPI_Send) || 
	   RecordHelper::is_event(r, event_MPI_Recv) || 
	   RecordHelper::is_event(r, event_MPI_Isend) || 
	   RecordHelper::is_event(r, event_MPI_Irecv) || 
	   RecordHelper::is_event(r, event_MPI_Alltoall) || 
	   RecordHelper::is_event(r, event_MPI_Alltoallv) || 
	   RecordHelper::is_event(r, event_MPI_Allreduce) || 
	   RecordHelper::is_event(r, event_MPI_Reduce) || 
	   RecordHelper::is_event(r, event_MPI_Bcast);
}

template<typename K, typename V>
std::vector<std::pair<K, V>> mapToVector(const std::unordered_map<K, V> &map) {
    return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

CommunicationAnalyzer::~CommunicationAnalyzer() {
	for (auto &r : _data_comm.items) {
		delete r;
	}
	for (auto &r : _data_wait.items) {
		delete r;
	}
	for(auto &i : _mpi_comm_2_event_map) {			  
		for(auto &j : i.second) {
			for(auto &k : j.second) {
				free(k.second);
			}
		}
    }
}

CommunicationAnalyzer::CommunicationAnalyzer(RecordReader& reader, RecordTraceCollection& collection, BacktraceCollection *backtraces, RankMetaCollection& metas, int topk, double threshold_lwt, double threshold_wtv, double threshold_S, double threshold_n, double threshold_t, bool pretty_print, const char *filename) 
	: _backtraces{*backtraces},
	  _num_top_k{topk},
	  _pretty_print{pretty_print},
	  _threshold_lwt{threshold_lwt},
	  _threshold_S{threshold_S} ,
	  _threshold_n{threshold_n} ,
	  _threshold_t{threshold_t} ,
	  _threshold_wtv{threshold_wtv}{
	auto mhz = get_tsc_freq_mhz();
	int nThreads;
    _data_comm.t_total = 0;
    _data_wait.t_total = 0;
	
	auto collection_vec = mapToVector(collection);
	int n = collection_vec.size();
	// std::cout << "nrank = " << n << std::endl;

	// can we use omp_get_num_threads to initialize the map array?
	#pragma omp parallel
    {
    #pragma omp master
    {
	nThreads = omp_get_num_threads();
    }
	}
	CommAnalyzeRecord data_comm_p[nThreads];
    CommAnalyzeRecord data_wait_p[nThreads];
	std::unordered_map<int, double> e2etime[nThreads];
	std::unordered_map<uint64_t, std::string> comm2name_p[nThreads];
	std::unordered_map<std::string, std::pair<int, uint64_t>> bt_funcfreq_map_p[nThreads];
	std::unordered_map<std::string, std::unordered_map<int, uint32_t>> comm_to_rank2barrierorder[nThreads];
    // for comm matrix
    std::unordered_map<int/*sender id*/, std::unordered_map<int/*receiver id*/, uint32_t/*comm times*/>> tid_to_send2count[nThreads];
	#pragma omp parallel
    {
    #pragma barrier
    int tid = omp_get_thread_num();
    #pragma omp for
	for(int i = 0; i < n; i++) {
		auto it = collection_vec[i];
		int rank = it.first;
		tid_to_send2count[tid][rank] = {};
		
		get_comm_world_from_meta(metas, rank, comm2name_p[tid]);

		RecordTrace& rtrace = *(it.second);
		for(auto ri=rtrace.begin(), re=rtrace.end(); ri!=re; ri=ri.next()) {
			record_t* r = ri.val();
			if(RecordHelper::is_process_start(r)) {
				e2etime[tid][rank] = r->timestamps.enter;
				continue;
			}
			if(RecordHelper::is_process_exit(r)) {
				e2etime[tid][rank] = tsc_duration_us(r->timestamps.exit - e2etime[tid][rank], mhz);
				continue;
			}
			if(RecordHelper::is_event(r, event_MPI_Barrier)) {
        		int size = RecordHelper::get_record_size(r, reader.get_pmu_event_num_by_rank(rank));
				record_barrier_t* rd = (record_barrier_t*) malloc(size);
				memcpy(rd, r, size);
				if(comm_to_rank2barrierorder[tid].find(comm2name_p[tid][rd->comm]) == comm_to_rank2barrierorder[tid].end()) comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]] = {};
				if(comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]].find(rank) == comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]].end()) comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]][rank] = 0;
				#pragma omp critical
				{
					_mpi_comm_2_event_map[comm2name_p[tid][rd->comm]][comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]][rank]].push_back({rank,rd});
				}
				comm_to_rank2barrierorder[tid][comm2name_p[tid][rd->comm]][rank]++;
			}
			if(RecordHelper::is_event(r, event_MPI_Comm_dup)) {
				record_comm_dup_t* rd = (record_comm_dup_t*) r;
				#pragma omp critical
				add_Comm_to_map(rank, rd->comm, rd->new_comm);
			}
			if(RecordHelper::is_event(r, event_MPI_Comm_split)) {
				record_comm_split_t* rd = (record_comm_split_t*) r;
				#pragma omp critical
				add_Comm_to_map(rank, rd->comm, rd->color, rd->new_comm);
			}
			if(RecordHelper::is_event(r, event_MPI_Wait)) {
				uint64_t dur = get_comm_duration(r, mhz);
				CommAnalyzeItem* lit = new CommAnalyzeItem(rank, r, dur, 0, reader.get_pmu_event_num_by_rank(rank));
				data_wait_p[tid].items.emplace_back(lit);
				data_wait_p[tid].t_total += dur;
			}
			if(record_is_comm(r))  {
				uint64_t dur = get_comm_duration(r, mhz);
			
				auto func_ctx = (backtrace_context_t) r->ctxt;
				const auto &bt_tree = *_backtraces[rank];
				auto enc = encode_backtrace(func_ctx, bt_tree);
				if(bt_funcfreq_map_p[tid].find(enc) == bt_funcfreq_map_p[tid].end()) {
					bt_funcfreq_map_p[tid][enc] = {0, 0};
				}
				bt_funcfreq_map_p[tid][enc].first++;
				bt_funcfreq_map_p[tid][enc].second+=dur;
				if (r->MsgType == event_MPI_Send || r->MsgType == event_MPI_Isend) {
					int dest = ((record_comm_t*) r)->dest;
					if (tid_to_send2count[tid][rank].find(dest) == tid_to_send2count[tid][rank].end()) {
						tid_to_send2count[tid][rank][dest] = 0;
					}
					tid_to_send2count[tid][rank][dest]++;
				}
					
				CommAnalyzeItem* lit = new CommAnalyzeItem(rank, r, dur, get_comm_volume(r), reader.get_pmu_event_num_by_rank(rank));
				data_comm_p[tid].items.emplace_back(lit);
				data_comm_p[tid].t_total += dur;
			}
		}
	}
	}

    std::ofstream f_out5;
    std::string output_dir(filename);
	
    f_out5.open(output_dir + std::string("/result.comm_mtx"));
	f_out5 << n << std::endl;
	for (auto &sender2count : tid_to_send2count) {
		for (auto &recv2times : sender2count) {
			// std::cout << "sender id = " << recv2times.first << std::endl;
			for (auto &r : recv2times.second) {
				// std::cout << "comm with receiver id = " << r.first << " " << r.second << "times. " << std::endl;
				f_out5 << recv2times.first << "," << r.first << "," << r.second << std::endl;
			}
		}
	}
	for(auto &t : data_comm_p) {
		for(auto &item : t.items) {
			_data_comm.items.emplace_back(item);
			_data_comm.t_total += item->duration;
		}
	}
	for(auto &t : data_wait_p) {
		for(auto &item : t.items) {
			_data_wait.items.emplace_back(item);
			_data_wait.t_total += item->duration;
		}
	}
    for(auto &item : _data_comm.items) {
		item->time_percentage = ((item->duration) / (double) _data_comm.t_total);
    }
    for(auto &item : _data_wait.items) {
		item->time_percentage = ((item->duration) / (double) _data_wait.t_total);
    }
    for(auto &e : e2etime) {
		for(auto &e2e : e) {
			_rank_e2etime_map[e2e.first] = e2e.second;
		}
    }	
    for(auto &b : bt_funcfreq_map_p) {
		for(auto &bt : b) {
			_bt_funcfreq_map[bt.first] = bt.second;
		}
    }

	dump_report(filename);
}

void CommunicationAnalyzer::print_long_wait(std::ofstream &f_out) {
	auto mhz = get_tsc_freq_mhz();
    f_out << "====== Long Wait Communication ======\n";
	std::vector<CommAnalyzeItem*> ab_comm_items, ab_wait_items;
	for(auto &i : _data_comm.items) {
		if(i->time_percentage >= _threshold_lwt)
			ab_comm_items.push_back(i);
	}
	for(auto &i : _data_wait.items) {
		if(i->time_percentage >= _threshold_wtv)
			ab_wait_items.push_back(i);
	}
    std::sort(ab_comm_items.begin(), ab_comm_items.end(),
                  [&](const CommAnalyzeItem *i1, const CommAnalyzeItem *i2) {
                      return i1->time_percentage > i2->time_percentage;
                  });
    std::sort(ab_wait_items.begin(), ab_wait_items.end(),
                  [&](const CommAnalyzeItem *i1, const CommAnalyzeItem *i2) {
                      return i1->duration > i2->duration;
                  });
    if(!_pretty_print) {

    f_out << "rank,event,time,percentage,backtrace" << std::endl;

    int num = 0;
    for(const auto &item : ab_comm_items) {
        if(num == _num_top_k) {
            break;	
        }
        
		f_out << item->rank << "," 
			<< RecordHelper::get_record_name(item->data) << ","
			<< item->duration << ","
			<< item->time_percentage << ",";

		print_backtrace(f_out, _backtraces, item);
		num++;
    }

    num = 0;
    for(const auto &item : ab_wait_items) {
        if(num == _num_top_k) {
            break;	
        }
        
		f_out << item->rank << "," 
			<< RecordHelper::get_record_name(item->data) << ","
			<< item->duration << ","
			<< item->time_percentage << ",";
				
		print_backtrace(f_out, _backtraces, item);
		num++;
    }

    for(const auto &i : _mpi_comm_2_event_map) {
		for(const auto &j : i.second) {
			for(const auto &k : j.second) {
				f_out << i.first << "," 
					<< k.first << ","
					<< "MPI_Barrier" << ","
					<< get_comm_duration((record_t*)k.second, mhz) << ",";

				std::vector<const char*> bt_vec;
				_backtraces[k.first]->backtrace_get_context_string_vec(k.second->record.ctxt, 20, bt_vec);

				for(auto c : bt_vec) {
					f_out << c << ";";
				}
				f_out << std::endl;
			}
		}
    }
    } else {
        int num = 0;
		for(const auto &item : ab_comm_items) {
			if(num == _num_top_k) {
				break;	
			}
			num++;
			
			f_out	<< "rank: " << std::setw(5) << item->rank << ", "
				<< std::string("Event Name: ") + RecordHelper::get_record_name(item->data) + std::string("\n")
				<< "time: " << item->duration << ", "
				<< "percentage: " << item->time_percentage << std::endl
				<< "backtrace: " << _backtraces[item->rank]->backtrace_get_context_string(item->data->ctxt, 20) << std::endl
				<< "======\n";
		}

		num = 0;
		for(const auto &item : ab_wait_items) {
			if(num == _num_top_k) {
				break;	
			}
			num++;
		
			f_out	<< "rank: " << std::setw(5) << item->rank << ", "
				<< std::string("Event Name: ") + RecordHelper::get_record_name(item->data) + std::string("\n")
				<< "time: " << item->duration << ", "
				<< "percentage: " << item->time_percentage << std::endl
				<< "backtrace: " << _backtraces[item->rank]->backtrace_get_context_string(item->data->ctxt, 20) << std::endl
				<< "======\n";
		}

		for(const auto &i : _mpi_comm_2_event_map) {
			f_out << "comm: " << i.first << std::endl;
			for(const auto &j : i.second) {
				for(const auto &k : j.second) {
				f_out << "rank: " << k.first << ", barrier dur: " << get_comm_duration((record_t*)k.second, mhz) << std::endl
					<< "backtrace: " << _backtraces[k.first]->backtrace_get_context_string(k.second->record.ctxt, 20) << std::endl;
				}
				f_out << "======" << std::endl;
			}
		}
    }
}

void CommunicationAnalyzer::print_high_proportion_MPI_calls(std::ofstream &f_out) {
    f_out << "====== High proportion MPI calls ======\n";
	std::vector<CommAnalyzeItem*> ab_comm_items;
	for(auto &i : _data_comm.items) {
		ab_comm_items.push_back(i);
	}
    std::sort(ab_comm_items.begin(), ab_comm_items.end(),
                  [&](const CommAnalyzeItem *i1, const CommAnalyzeItem *i2) {
                      return (i1->duration / (double)_rank_e2etime_map[i1->rank]) > (i2->duration / (double)_rank_e2etime_map[i2->rank]);
                  });

    if(!_pretty_print) {
		f_out << "rank,event,proportion,backtrace" << std::endl;

		int num = 0;
		for(const auto &item : ab_comm_items) {
			if(num == _num_top_k) {
				break;	
			}
			f_out << item->rank << "," 
					<< RecordHelper::get_record_name(item->data) << ","
					<< (item->duration / (double)_rank_e2etime_map[item->rank]) << ",";
						
			print_backtrace(f_out, _backtraces, item);
			num++;
    	}
    } else {
    	int num = 0;
		for(const auto &item : ab_comm_items) {
			if(num == _num_top_k) {
				break;	
			}
			num++;
				
			f_out	<< "rank: " << std::setw(5) << item->rank << ", "
				<< std::string("Event Name: ") + RecordHelper::get_record_name(item->data) + std::string("\n")
				<< "proportion: " << item->duration << "/" << _rank_e2etime_map[item->rank] << "=" << (item->duration / (double)_rank_e2etime_map[item->rank]) << std::endl
				<< "backtrace: " << _backtraces[item->rank]->backtrace_get_context_string(item->data->ctxt, 20) << std::endl
				<< "======\n";
		}
    }
    
}

void CommunicationAnalyzer::print_high_comm_volume_MPI_calls(std::ofstream &f_out) {
    f_out << "====== High communication volume MPI calls ======\n";
	std::vector<CommAnalyzeItem*> ab_comm_items;
	for(auto &i : _data_comm.items) {
		if(i->volume >= _threshold_S)
			ab_comm_items.push_back(i);
	}
    std::sort(ab_comm_items.begin(), ab_comm_items.end(),
                  [&](const CommAnalyzeItem *i1, const CommAnalyzeItem *i2) {
                      return i1->volume > i2->volume;
                  });
    if(!_pretty_print) {

		f_out << "rank,event,volume,backtrace" << std::endl;

		int num = 0;
		for(const auto &item : ab_comm_items) {
			if(num == _num_top_k) {
				break;	
			}

			f_out << item->rank << "," 
				<< RecordHelper::get_record_name(item->data) << ","
				<< item->volume << ",";
					
			print_backtrace(f_out, _backtraces, item);
			num++;
		}
    } else {
        int num = 0;
		for(const auto &item : ab_comm_items) {
			if(num == _num_top_k) {
				break;	
			}
			num++;

			f_out	<< "rank: " << std::setw(5) << item->rank << ", "
			<< std::string("Event Name: ") + RecordHelper::get_record_name(item->data) + std::string("\n")
			<< "volume: " << item->volume << std::endl
			<< "backtrace: " << _backtraces[item->rank]->backtrace_get_context_string(item->data->ctxt, 20) << std::endl
			<< "======\n";
    	}
    }
    
}

void CommunicationAnalyzer::print_high_freq_MPI_calls(std::ofstream &f_out) {
    f_out << "====== High communication frequency MPI calls ======\n";
    if(!_pretty_print) {
    f_out << "rank,event,time_avg,communication times,backtrace" << std::endl;

    int num = 0;
    for(const auto &item : _data_comm.items) {
        auto func_ctx = (backtrace_context_t) item->data->ctxt;
        const auto &bt_tree = *_backtraces[item->rank];
        auto enc = encode_backtrace(func_ctx, bt_tree);
		auto comm_times = _bt_funcfreq_map[enc].first;
		auto time_avg = (_bt_funcfreq_map[enc].second / (double)comm_times);
        if(comm_times < _threshold_n || time_avg > _threshold_t) {
            continue;
        }
		if(num == _num_top_k) break;
		f_out << item->rank << "," 
	      << RecordHelper::get_record_name(item->data) << ","
	      << time_avg << ","
	      << comm_times << ",";
		print_backtrace(f_out, _backtraces, item);
		num++;
    }
    } else {
    	int num = 0;
    	for(const auto &item : _data_comm.items) {
			auto func_ctx = (backtrace_context_t) item->data->ctxt;
			const auto &bt_tree = *_backtraces[item->rank];
			auto enc = encode_backtrace(func_ctx, bt_tree);
			auto comm_times = _bt_funcfreq_map[enc].first;
			auto time_avg = (_bt_funcfreq_map[enc].second / (double)comm_times);
			if(comm_times < _threshold_n || time_avg > _threshold_t) {
				continue;	
			}
			if(num == _num_top_k) break;

			f_out	<< "rank: " << std::setw(5) << item->rank << ", "
			<< std::string("Event Name: ") + RecordHelper::get_record_name(item->data) + std::string("\n")
			<< "time_avg: " << time_avg << ", communication times:" << comm_times <<std::endl
			<< "backtrace: " << _backtraces[item->rank]->backtrace_get_context_string(item->data->ctxt, 20) << std::endl
			<< "======\n";
			num++;
    	}	
    }
    
}

void CommunicationAnalyzer::dump_report(const char *filename) {
    std::ofstream f_out1, f_out2, f_out3, f_out4;
    std::string output_dir(filename);
	
    f_out1.open(output_dir + std::string("/result.long_wait"));
    f_out2.open(output_dir + std::string("/result.high_prop"));
    f_out3.open(output_dir + std::string("/result.high_vol"));
    f_out4.open(output_dir + std::string("/result.high_freq"));
#pragma omp parallel num_threads(4)
	{
#pragma omp sections
		{
#pragma omp section
			print_long_wait(f_out1);
#pragma omp section
			print_high_proportion_MPI_calls(f_out2);
#pragma omp section
			print_high_comm_volume_MPI_calls(f_out3);
#pragma omp section
			print_high_freq_MPI_calls(f_out4);
		}
	}
}
