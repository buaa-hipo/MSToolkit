import matplotlib
matplotlib.use('Agg')  # 启用非GUI后端（关键修改）
import matplotlib.pyplot as plt
import numpy as np
import csv
import os
import sys
from mpi4py import MPI

def draw_csv(csv_file, image_file_path):
    rank = csv_file.strip().split(".csv")[0].split("rank")[-1]
    
    timestamp, alloc_size, free_size, memory_usage, usage_ratio = [], [], [], [], []
    with open(csv_file, mode='r', encoding='utf-8') as file:
        csv_reader = csv.DictReader(file)
        for row in csv_reader:
            timestamp.append(int(row['timestamp']))
            alloc_size.append(int(row['alloc_size (Bytes)']))
            free_size.append(int(row['free_size (Bytes)']))
            memory_usage.append(int(row['memory_usage (Bytes)']))
            usage_ratio.append(float(row['usage_ratio']))
    
    # 单位转换（MB和百分比）
    timestamp = np.array(timestamp)
    alloc_size = np.array(alloc_size) / 1024**2
    free_size = np.array(free_size) / 1024**2
    memory_usage = np.array(memory_usage) / 1024**2
    usage_ratio = np.array(usage_ratio) * 100
    
    # 数据采样（保留全部数据）
    step = 1
    timestamp = timestamp[::step]
    alloc_size = alloc_size[::step]
    free_size = free_size[::step]
    memory_usage = memory_usage[::step]
    usage_ratio = usage_ratio[::step]
    
    # 绘制双子图
    plt.figure(figsize=(10, 8))
    
    plt.subplot(2, 1, 1)
    plt.step(timestamp, alloc_size, label='Alloc Size (MB)', color='blue', where='post')
    plt.step(timestamp, free_size, label='Free Size (MB)', color='green', where='post')
    plt.step(timestamp, memory_usage, label='Memory Usage (MB)', color='red', where='post')
    plt.xlabel('Timestamp')
    plt.ylabel('Memory Size (MB)')
    plt.title(f'{"Accl " if "accl" in csv_file else ""}Memory Metrics For Rank {rank}')
    plt.legend()
    plt.grid()
    
    plt.subplot(2, 1, 2)
    plt.step(timestamp, usage_ratio, label='Usage Ratio (%)', color='orange', where='post')
    plt.xlabel('Timestamp')
    plt.ylabel('Usage Ratio (%)')
    plt.title(f'{"Accl " if "accl" in csv_file else ""}Memory Usage Ratio For Rank {rank}')
    plt.legend()
    plt.grid()
    
    plt.tight_layout()
    plt.savefig(image_file_path, dpi=300, bbox_inches='tight')
    plt.close()  # 显式关闭图形对象释放内存

if __name__ == '__main__':
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    if len(sys.argv) != 3:
        if rank == 0:
            sys.exit(1)

    csv_dir = sys.argv[1]
    image_dir = sys.argv[2]

    # 预处理输入输出目录
    csv_files = [os.path.join(csv_dir, f) for f in os.listdir(csv_dir) if f.endswith('.csv')]
    csv_files.sort(key=lambda x: int(x.split('rank')[-1].split('.')[0]))

    os.makedirs(image_dir, exist_ok=True)

    # 划分任务
    chunk_size = (len(csv_files) + size - 1) // size
    start = rank * chunk_size
    end = min((rank + 1) * chunk_size, len(csv_files))
    for idx in range(start, end):
        csv_file = csv_files[idx]
        image_path = os.path.join(image_dir, os.path.basename(csv_file).replace('.csv', '.png'))
        print(f"[进程{rank}] 处理文件: {csv_file} -> {image_path}")
        draw_csv(csv_file, image_path)

    comm.Barrier()
    if rank == 0:
        print("所有绘图任务完成")    