import matplotlib.pyplot as plt
import re
import numpy as np
import seaborn
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

    return local_times, node1_times

# 绘制曲线
def plot_times(local_times, node1_times):
    delta = [(local_times[i] - node1_times[i]) for i in range(len(local_times))]
    print(delta)
    time_err = np.array(delta)
    custom_bins = np.arange(-100, 0, 5)
    # 绘制差值的直方图
    plt.style.use("seaborn-v0_8-deep")
    plt.figure(dpi=200)
    print(f"均值={time_err.mean()},标准差={time_err.std()},最大值={time_err.max()},最小值={time_err.min()}")
    plt.hist(time_err, bins=custom_bins, edgecolor='black')
    plt.xlabel('误差/us')
    plt.ylabel('频数')
    plt.title('时间同步误差直方图')
    plt.show()

# 主函数
def main():
    file_path = 'sync_notify_event.txt'  # 替换为你的文件路径
    data = read_data_from_file(file_path)
    local_times, node1_times = extract_times(data)
    plot_times(local_times, node1_times)

if __name__ == "__main__":
    main()