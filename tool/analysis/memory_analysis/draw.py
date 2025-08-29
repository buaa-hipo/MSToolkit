import matplotlib
# matplotlib.use('Agg')  # Use a non-GUI backend for matplotlib
import matplotlib.pyplot as plt
import numpy as np
import csv
import os
import sys
from multiprocessing import Process , Manager,freeze_support

def draw_csv(csv_file, image_file_path,share_data):
    rank = csv_file.strip().split(".csv")[0].split("rank")[-1]
    
    timestamp = []
    alloc_size = []
    free_size = []
    memory_usage = []
    usage_ratio = []
    with open(csv_file, mode='r', encoding='utf-8') as file:
        csv_reader = csv.DictReader(file)
        for idx, row in enumerate(csv_reader):
            timestamp.append(int(row['timestamp']))
            alloc_size.append(int(row['alloc_size (Bytes)']))
            free_size.append(int(row['free_size (Bytes)']))
            memory_usage.append(int(row['memory_usage (Bytes)']))
            usage_ratio.append(float(row['usage_ratio']))
    timestamp = np.array(timestamp)
    alloc_size = np.array(alloc_size)/1024/1024
    free_size = np.array(free_size)/1024/1024
    memory_usage = np.array(memory_usage)/1024/1024
    usage_ratio = np.array(usage_ratio)*100
    
    # remove前后5%
    front = 0 * len(timestamp)
    back = 1 * len(timestamp)
    timestamp = timestamp[int(front):int(back)]
    alloc_size = alloc_size[int(front):int(back)]
    free_size = free_size[int(front):int(back)]
    memory_usage = memory_usage[int(front):int(back)]
    usage_ratio = usage_ratio[int(front):int(back)]
    
    #每隔几个取几个
    step = 1
    timestamp= timestamp[::step]
    alloc_size = alloc_size[::step]
    free_size = free_size[::step]
    memory_usage = memory_usage[::step]
    usage_ratio = usage_ratio[::step]
    
    # share_data['timestamp'][index] = timestamp
    # share_data['alloc_size'][index] = alloc_size
    # share_data['free_size'][index] = free_size
    # share_data['memory_usage'][index] = memory_usage
    # share_data['usage_ratio'][index] = usage_ratio
    
    plt.figure(figsize=(7.5, 6))
    plt.subplot(2, 1, 1)
    plt.step(timestamp, alloc_size, label='Alloc Size', color='blue', where='post')
    plt.step(timestamp, free_size, label='Free Size', color='green', where='post')
    plt.step(timestamp, memory_usage, label='Memory Usage', color='red', where='post')
    # plt.plot(timestamp, alloc_size, label='Alloc Size', color='blue')
    # plt.plot(timestamp, free_size, label='Free Size', color='green')
    # plt.plot(timestamp, memory_usage, label='Memory Usage', color='red')
    # plt.plot(timestamp, usage_ratio, label='Usage Ratio', color='orange')
    plt.xlabel('Timestamp')
    plt.ylabel('Memory Size (MB)')
    if csv_file.strip().split("/")[-1].startswith("accl"):
        plt.title(f'Accl Memory Size Over Time For Rank {rank}')
    else:
        plt.title(f'Memory Size Over Time For Rank {rank}')
    plt.legend()
    plt.grid()
    
    plt.subplot(2, 1, 2)
    plt.step(timestamp, usage_ratio, label='Usage Ratio', color='orange', where='post')
    # plt.plot(timestamp, usage_ratio, label='Usage Ratio', color='orange')
    plt.xlabel('Timestamp')
    plt.ylabel('Usage Ratio(%)')
    if csv_file.strip().split("/")[-1].startswith("accl"):
        plt.title(f'Accl Usage Ratio Over Time For Rank {rank}')
    else:
        plt.title(f'Usage Ratio Over Time For Rank {rank}')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    
    plt.savefig(image_file_path, dpi=300)
    
def process_draw_csv(csv_file_list, image_dir,start,end,share_data):
    for index in range(start, end):
        csv_file = csv_file_list[index]
        image_file_path = os.path.join(image_dir, os.path.basename(csv_file).replace('.csv', '.png'))
        print(f"Start processing {csv_file}")
        draw_csv(csv_file, image_file_path,share_data)
        print(f"Finished processing {csv_file}")


if __name__ == '__main__':
    csv_dir = sys.argv[1]
    image_dir = sys.argv[2]
    csv_file_list = os.listdir(csv_dir)
    csv_file_list = [os.path.join(csv_dir, i) for i in csv_file_list if i.endswith('.csv')]
    csv_file_list = sorted(csv_file_list, key=lambda x: int(x.split('/')[-1].split('.')[-2].split('k')[-1]))
    
    if not os.path.exists(image_dir):
        os.makedirs(image_dir)
    else:
        for file in os.listdir(image_dir):
            file_path = os.path.join(image_dir, file)
            if os.path.isfile(file_path):
                os.remove(file_path)
    
    manager = Manager()
    share_data = manager.dict({
        'timestamp': manager.list([None] * len(csv_file_list)),
        'alloc_size': manager.list([None] * len(csv_file_list)),
        'free_size': manager.list([None] * len(csv_file_list)),
        'memory_usage': manager.list([None] * len(csv_file_list)),
        'usage_ratio': manager.list([None] * len(csv_file_list)),
    })

    
    process_list = []
    process_num = max(1, int(os.cpu_count()*float(sys.argv[3])))
    chunk_size = (len(csv_file_list) + process_num - 1) // process_num
    for i in range(process_num):
        start = i * chunk_size
        end = min((i + 1) * chunk_size, len(csv_file_list))
        process_list.append(Process(target=process_draw_csv, args=(csv_file_list, image_dir, start, end,share_data)))

    for process in process_list:
        process.start()
    for process in process_list:
        process.join()
        
    print("主线程结束")    
    
    # def sum_vector(vec):
    #     max_len = max(len(i) for i in vec)
    #     for i in vec:
    #         i.extend([0] * (max_len - len(i)))
    #     re = np.zeros(max_len)
    #     for i in vec:
    #         re += np.array(i)
    #     return re/len(vec)
    
    # timestamp_all = sum_vector(share_data['timestamp'])
    # alloc_size_all = sum_vector(share_data['alloc_size'])
    # free_size_all = sum_vector(share_data['free_size'])
    # memory_usage_all = sum_vector(share_data['memory_usage'])
    # usage_ratio_all = sum_vector(share_data['usage_ratio']) 

    # print("主线程结束")