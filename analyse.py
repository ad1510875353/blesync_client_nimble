import matplotlib.pyplot as plt
import re
plt.rcParams['font.family'] = 'SimHei'  # 替换为你选择的字体
# 读取文本文件
def read_data_from_file(file_path):
    with open(file_path, 'r') as file:
        data = file.read()
    return data

# 提取时间数据
def extract_times(data):
    local_times = []
    node1_times = []
    pattern = r'Local time : (\d+)\nNode1 time : (\d+)'
    matches = re.findall(pattern, data)
    for match in matches:
        local_times.append(int(match[0]))
        node1_times.append(int(match[1]))
    

    local_times = [(i - local_times[0])/1000 for i in local_times]
    node1_times = [(i - node1_times[0])/1000 for i in node1_times]
    return local_times, node1_times

# 绘制曲线
def plot_times(local_times, node1_times):

    x = [i*2 for i in range(len(local_times))]
    plt.plot(x,local_times, label='Local Time')
    plt.plot(x,node1_times, label='Node1 Time')

    plt.legend()
    plt.title('time')
    plt.xlabel('ms')
    plt.ylabel('s')
    plt.grid(True)
    plt.show()

    delta = [(node1_times[i] - local_times[i]) for i in range(len(local_times))]
    plt.plot(x,delta)
    plt.title('Time Bias')
    plt.xlabel('s')
    plt.ylabel('ms')
    plt.grid(True)
    plt.show()

# 主函数
def main():
    file_path = 'time_shift_30Hz.txt'  # 替换为你的文件路径
    data = read_data_from_file(file_path)
    local_times, node1_times = extract_times(data)
    plot_times(local_times, node1_times)

if __name__ == "__main__":
    main()