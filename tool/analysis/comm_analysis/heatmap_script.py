import sys
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import csv

def generate_heatmap(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()
    nrank = int(lines[0])
    matrix = np.zeros((nrank, nrank))
    for i in range(1, len(lines)):
        str_tup = lines[i].split(',')
        sender = int(str_tup[0])
        receiver = int(str_tup[1])
        comm_times = int(str_tup[2])
        matrix[sender,receiver] = comm_times
        
    # print(matrix)

    # Create a heatmap
    plt.figure(figsize=(8, 6))
    sns.heatmap(matrix, annot=False, cmap='YlGnBu', cbar=True)
    # sns.heatmap(matrix, annot=True, cmap='YlGnBu', cbar=True)

    # Add labels
    plt.title('MPI Send/Recv Heatmap')
    plt.xlabel('Receiver Process')
    plt.ylabel('Sender Process')

    # Show the heatmap
    # plt.show()
    plt.savefig('heatmap.png')

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python heatmap_script.py <matrix_data_file>")
        sys.exit(1)

    matrix_file = sys.argv[1]
    generate_heatmap(matrix_file)

