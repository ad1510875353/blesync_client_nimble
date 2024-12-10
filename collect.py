# -*- coding: utf-8 -*-
import serial
import time

def read_and_print_serial(port, baud_rate, output_file):
    try:
        # 打开串口
        ser = serial.Serial(port, baud_rate, timeout=1)

        # 打开文件用于保存输出
        with open(output_file, 'w', encoding='utf-8') as file:
            while True:
                # 读取串口数据
                data = ser.readline().decode('utf-8')

                # 打印到控制台
                print(data, end='')
                data = data[:-1]
                # 将数据写入文件
                file.write(data)
                file.flush()

    except Exception as e:
        print(f"发生异常: {e}")
        print("保存输出到文件...")
        # 将异常信息写入文件
        with open(output_file, 'a',encoding='utf-8') as file:
            file.write(f"\n发生异常: {e}\n")
        print("保存完成")


if __name__ == "__main__":
    # 串口配置
    serial_port = "COM8"  # 根据你的实际串口设置
    baud_rate = 115200  # 根据你的实际波特率设置

    # 输出文件名
    output_file = "sync_ntp.txt"

    # 读取并打印串口输出，遇到异常时保存为txt
    read_and_print_serial(serial_port, baud_rate, output_file)