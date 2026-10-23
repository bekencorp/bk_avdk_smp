#!/bin/bash
# BK7259 4G ECM 自动化测速快速启动脚本

echo "========================================"
echo "  BK7259 4G ECM 自动化测速工具"
echo "========================================"
echo ""

# 检查串口设备
if [ ! -e "/dev/cu.usbserial-120" ]; then
    echo "⚠️  警告: 串口设备 /dev/cu.usbserial-120 不存在"
    echo ""
    echo "可用的串口设备："
    ls -l /dev/cu.* /dev/tty.* 2>/dev/null | grep -E "usbserial|usbmodem|tty.usb"
    echo ""
    echo "请修改 speed_test_monitor.py 中的 SERIAL_PORT 参数"
    exit 1
fi

# 检查 Python 依赖
python3 -c "import serial" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "✗ pyserial 未安装"
    echo "正在安装 pyserial..."
    pip3 install pyserial
fi

# 运行测速脚本
echo "✓ 准备就绪，开始测速..."
echo ""
python3 speed_test_monitor.py

echo ""
echo "测速完成！"
