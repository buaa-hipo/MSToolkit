import numpy as np
from scipy.interpolate import griddata
import matplotlib.pyplot as plt
import argparse
import sys
import shutil
import os

NSEC = 1
USEC = 1000
MSEC = 1000*USEC
SEC  = 1000*MSEC 

TIME_UNIT = MSEC
TIME_UNIT_STR = "ms"

def read_host_csv(host_csv):
    hosts = {}
    host_list = {} 
    with open(host_csv) as f:
        for line in f:
            items = line.split(',')
            rank = int(items[0])
            host = items[1][:-1]
            hosts[rank] = host
            host_list[host] = 0
    hlist = list(host_list.keys())
    hlist.sort()
    return hosts, hlist

def read_csv(csv_file, time_threshold):
    S = []
    E = []
    D = []
    off = 0
    with open(csv_file) as f:
        for line in f:
            x = []
            y = []
            data = []
            items = line[:-1].split(',')
            rank = int(items[0])
            while len(S)<=rank:
                S.append([])
                E.append([])
                D.append([])
            items = items[1:]
            if len(items)<3:
                # invalid line, ignore it
                continue
            for i in range(0, len(items), 3):
                if len(items)-i < 3:
                    break
                start=float(items[i])/TIME_UNIT
                # as the offset is unknown when loading the data we need to load all data
                # if time_threshold>0 and start>time_threshold:
                #     break
                x.append(float(items[i])/TIME_UNIT)
                y.append(float(items[i+1])/TIME_UNIT)
                data.append(float(items[i+2]))
            S[rank] = x
            E[rank] = y
            D[rank] = data
            minS = min(x)
            if off==0 or off>minS:
                off = minS
    for i in range(len(S)):
        for j in range(len(S[i])):
            S[i][j] -= off
            E[i][j] -= off
    #print(off, min(min(E)))
    return S, E, D, off

def draw_heatmap(csv_file, fig_file, time_threshold, method, res, node_level, host_csv, en_start, step=0.005):
    read_host_csv(host_csv)
    print("Loading csv file... %s" % csv_file)
    S, E, D, off = read_csv(csv_file, time_threshold)
    m = max(max(E))
    n = len(S)
    
    if time_threshold>0 and m>time_threshold:
        m=time_threshold
    
    data = []
    
    if method=="GRID":
        print("GridSize: {0}x{1}".format(m/step,n))
        
        for i in range(n):
            grid_x, grid_y = np.mgrid[0:m+step:step, i:i+1]
            grid = griddata((S[i]+E[i]+[max(E[i])+0.01], [i for _ in range(len(S[i])+len(E[i])+1)]), D[i]+D[i]+[0], (grid_x, grid_y), method='nearest')
            data.append(grid.T[0])

        print("Draw heatmap...")
        f, ax = plt.subplots(1, 1, figsize=(12, 6))
        im = ax.imshow(data, aspect='auto', interpolation ='nearest', cmap='Blues', vmin=0, vmax=1)
        ax.set_xlabel("Time (%s)" % TIME_UNIT_STR)

        if node_level:
            ax.set_ylabel("Node Name")
        else:
            ax.set_ylabel("MPI Process")
        
        ax.set_yticklabels([])
        xticks = ax.get_xticks()
        xticklabels = []
        for i in range(len(xticks)):
            xticklabels.append(xticks[i]*step)
        ax.set_xticks(xticks)
        ax.set_xticklabels(xticklabels)
        ax.set_xlim(0, m/step)
    elif method=="AVG":
        print("AVG Smoothing...")
        N_grid = int(m / res)
        mask_data = []
        skip_cnt = 0
        for i in range(n):
            ind = 0
            dat = 0
            cnt = 0
            start = 0
            data_rank = []
            mask_rank = []
            for j in range(N_grid):
                end = start + res
                if ind<len(S[i]) and S[i][ind]<start and E[i][ind]>start:
                    if D[i][ind]>0:
                        dur = 1#E[i][ind] - start
                        dat += D[i][ind] * dur
                        cnt += dur
                    ind += 1
                while ind<len(S[i]) and S[i][ind]>=start and S[i][ind]<end:
                    if E[i][ind]<end:
                        if D[i][ind]>0:
                            dur = E[i][ind] - S[i][ind]
                            dat += D[i][ind] * dur
                            cnt += dur
                        ind += 1
                    else:
                        if D[i][ind]>0:
                            dur = end - S[i][ind]
                            dat += D[i][ind] * dur
                            cnt += dur
                        break
                if cnt==0 and start >= en_start:
                    data_rank.append(1)
                    mask_rank.append(0)
                elif start >= en_start:
                    data_rank.append(dat/cnt)
                    mask_rank.append(1)
                else:
                    skip_cnt += 1
                start = end
            data.append(data_rank)
            mask_data.append(mask_rank)
        yticklabels = []
        yticks = None
        if node_level:
            print("Smoothing at node level")
            hosts, host_list = read_host_csv(host_csv)
            data_node = {}
            mask_node = {}
            for rank,host in hosts.items():
                if host not in data_node.keys():
                    data_node[host] = [ data[rank][i]*mask_data[rank][i] for i in range(len(data[rank])) ]
                    mask_node[host] = mask_data[rank]
                else:
                    for i in range(0, len(data[rank])):
                        data_node[host][i] += data[rank][i] * mask_data[rank][i]
                        mask_node[host][i] += mask_data[rank][i]
            data = []
            cnt = 0
            yticks = []
            for h in host_list:
                d = data_node[h]
                m = mask_node[h]
                for i in range(0, len(d)):
                    if m[i]>0:
                        d[i] /= m[i]
                        m[i] = 1
                    else:
                        d[i] = 1
                yticks.append(cnt)
                yticklabels.append(h)
                data.append(d)
                cnt += 1

        data_max = max(max(d) for d in data)
        for d in data:
            for i in range(len(d)):
                d[i] = d[i] / data_max

        print("Draw heatmap...")
        f, ax = plt.subplots(1, 1, figsize=(12, 6))
        im = ax.imshow(data, aspect='auto', interpolation ='nearest', cmap='Blues', vmin=0, vmax=1)
        ax.set_xlabel("Time (%s)" % TIME_UNIT_STR)
        if node_level:
            ax.set_ylabel("Node Name")
        else:
            ax.set_ylabel("MPI Process")
        if yticks:
            ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels)
        xticks = ax.get_xticks()
        xticklabels = []
        for i in range(len(xticks)):
            xticklabels.append(xticks[i]*res)
        ax.set_xticks(xticks)
        ax.set_xticklabels(xticklabels)
        ax.set_xlim(0, N_grid)
    else:
        print("Unknown smooth method: ", method)
        exit(1)

    # Create colorbar
    cbar = ax.figure.colorbar(im, ax=ax)

    f.savefig(fig_file, bbox_inches = 'tight')
    print("Figure saved to %s" % fig_file)
    
def main():
    parser = argparse.ArgumentParser(description='Draw heatmap from csv generated by variance_anaylsis.')
    parser.add_argument('--input', help='the variance directory containing csv generated by variance_analysis', default='variance')
    parser.add_argument('--output', help='output directory', default='heatmaps/')
    parser.add_argument('--time', help='time threshold (%s) to cutoff the variance heatmap. 0 for full trace.' % TIME_UNIT_STR, default=0)
    parser.add_argument('--resolution', help='window resolution for smoothing the variance heatmap. Default is 100 (ms).', default=100)
    parser.add_argument('--smooth_method', help='smooth method used for drawing heatmap. Default is AVG. Candidate: AVG, GRID.', default='AVG')
    parser.add_argument('--node_level', help='Merge by node level.', action="store_true", default=False)
    parser.add_argument('--comm', help='draw heatmap for communication.', action="store_true", default=False)
    parser.add_argument('--calc', help='draw heatmap for calculation.', action="store_true", default=False)
    parser.add_argument('--accl-calc', help='draw heatmap for ACCL calculation.', action="store_true", default=False)
    parser.add_argument('--accl-memcpy', help='draw heatmap for ACCL memcpy (H2D,D2H,D2D).', action="store_true", default=False)
    parser.add_argument('--format', help='png,pdf,jpg, default is png.', default="png")
    parser.add_argument('--start', help='start time', default=0)
    args = parser.parse_args()

    if not args.calc and not args.comm:
        print("Specify at least one of --comm or --calc!")
        exit(1)
    
    if not os.path.exists(args.output):
        os.mkdir(args.output, 0o777)
        print("Directory %s created" % args.output)
    
    time = int(args.time)
    
    if args.comm:
        draw_heatmap(args.input+"/comm.csv", args.output+"/comm."+args.format, time, args.smooth_method, int(args.resolution), args.node_level, args.input+"/host.csv", int(args.start))
    if args.calc:
        draw_heatmap(args.input+"/calc.csv", args.output+"/calc."+args.format, time, args.smooth_method, int(args.resolution), args.node_level, args.input+"/host.csv", int(args.start))
    if args.accl_calc:
        draw_heatmap(args.input+"/accl_calc.csv", args.output+"/accl_calc."+args.format, time, args.smooth_method, int(args.resolution), args.node_level, args.input+"/host.csv", int(args.start))
    if args.accl_memcpy:
        draw_heatmap(args.input+"/accl_memcpy.csv", args.output+"/accl_memcpy."+args.format, time, args.smooth_method, int(args.resolution), args.node_level, args.input+"/host.csv", int(args.start))
    
main()
