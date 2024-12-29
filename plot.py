import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 指定中文字体路径
font_path = '/System/Library/Fonts/PingFang.ttc'
font_prop = fm.FontProperties(fname=font_path)

# 设置全局字体并加粗
plt.rcParams['font.family'] = font_prop.get_name()  # 设置字体
plt.rcParams['axes.unicode_minus'] = False  # 设置负号显示
plt.rcParams['font.weight'] = 'bold'  # 设置全局字体加粗

# 数据
data = {
    "进程数": [1, 2, 4, 8],
    "串行时间": [0.368778, 0.379198, 0.396716, 0.665043],
    "并行时间": [0.403081, 0.209736, 0.116914, 0.14774],
    "加速比": [0.913697, 1.808002, 3.43015, 4.94704],
}

# 创建图形和坐标轴 (去掉加速比的部分)
fig, ax1 = plt.subplots()

# 绘制串行时间和并行时间
ax1.set_xlabel("进程数", fontweight='bold')  # 加粗x轴标签
ax1.set_ylabel("时间（秒）", color="tab:blue", fontweight='bold')  # 加粗y轴标签
ax1.plot(data["进程数"], data["串行时间"], marker="o", label="串行时间", color="tab:blue")
ax1.plot(data["进程数"], data["并行时间"], marker="s", label="并行时间", color="tab:cyan")
ax1.tick_params(axis="y", labelcolor="tab:blue")

# 调整第一个图例位置到左上角
ax1.legend(loc="upper left", bbox_to_anchor=(0, 1), borderaxespad=0.)

# 设置网格
plt.grid(True, linestyle="--", alpha=0.7)

# 设置标题并加粗
# plt.title("表1 相同图像不同进程数对比实验", fontweight='bold')

# 显示图形
plt.tight_layout()
plt.show()

# 创建单独的加速比图形
fig2, ax2 = plt.subplots()

# 绘制加速比
ax2.set_xlabel("进程数", fontweight='bold')  # 加粗x轴标签
ax2.set_ylabel("加速比", color="tab:red", fontweight='bold')  # 加粗y轴标签
ax2.plot(data["进程数"], data["加速比"], marker="^", label="加速比", color="tab:red")
ax2.tick_params(axis="y", labelcolor="tab:red")

# 设置网格
plt.grid(True, linestyle="--", alpha=0.7)

# 设置标题并加粗
# plt.title("表2 加速比随进程数变化", fontweight='bold')

# 显示加速比图形
plt.tight_layout()
plt.show()
