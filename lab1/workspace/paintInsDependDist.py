import sys
import os
import csv
from matplotlib import pyplot as plt

# filename = 'insDependDist.csv'

MAX_DISPLAY = int(sys.argv[1]) if len(sys.argv) >= 2 else 45
filename = sys.argv[2] if len(sys.argv) >= 3 else 'insDependDist.csv'
SAVE_FILENAME = sys.argv[3] if len(sys.argv) >= 4 else None


def process(filename: str, color='red', marker='x', line=':'):
    with open(filename) as f:
        reader = csv.reader(f)
        depend_row = next(reader)

        allDepend = 0
        for i in range(MAX_DISPLAY):
            allDepend += int(depend_row[i])

        percent = []
        for i in range(MAX_DISPLAY):
            percent.append(int(depend_row[i]) / allDepend)

    plt.plot([n for n in range(2, MAX_DISPLAY + 1)],
             percent[1:MAX_DISPLAY], c=color, marker=marker, linestyle=line)

    plt.xlim(0, MAX_DISPLAY)
    # plt.ylim(0, percent[0] * 1.05)

    # plt.gca().xaxis.set_major_locator(plt.MultipleLocator(5))
    # plt.grid(linestyle=line)


plt.title("Instuction Dependance Distance Distribution", fontsize=16)
plt.xlabel('Dependence Distance', fontsize=12)
plt.ylabel('Percent of Dependence', fontsize=12)

colors = ['red', 'green', 'magenta', 'blue']
marks = ['.', ',', 'x', '1']
lines = ['-', '--', '-.', ':']

fig = plt.figure(dpi=150, figsize=(7, 4))

if filename == 'auto':
    files = [f for f in os.listdir() if f.endswith('.csv') and f != 'insDependDist.csv']
    commands = [f.replace('insDependDist-', '').replace('.csv', '') for f in files]
    SAVE_FILENAME = "imgs/insDependDist-all.png"
    # print("commands:", commands)
    for i, command in enumerate(commands):
        process(filename=f"insDependDist-{command}.csv", color=colors[i % len(colors)], marker=marks[i % len(marks)], line=lines[i % len(lines)])
    plt.legend(commands)
else:
    process(filename)

if SAVE_FILENAME is None:
    plt.show()
else:
    plt.savefig(SAVE_FILENAME)
