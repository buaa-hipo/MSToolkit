import sys
import numpy as np
import matplotlib.pyplot as plt
import csv

def read_comm_mtx(filename, total_rank, merge_list):
    with open(filename, 'r') as file:
        lines = file.readlines()
    nrank = int(lines[0])
    for i in range(1, len(lines)):
        str_tup = lines[i].split(',')
        sender = int(str_tup[0])
        receiver = int(str_tup[1])
        comm_times = int(str_tup[2])
        merge_list.append(f'{sender+total_rank}, {receiver+total_rank}, {comm_times}')
    total_rank += nrank
    return total_rank, merge_list
        

if __name__ == '__main__':
    file_num = len(sys.argv) - 1
    merge_list = []
    total_rank = 0
    for i in range(0, file_num):
        matrix_file = sys.argv[i+1]
        total_rank, merge_list = read_comm_mtx(matrix_file, total_rank, merge_list)
    print(total_rank)
    for i in range(0, len(merge_list)):
        print(merge_list[i])

