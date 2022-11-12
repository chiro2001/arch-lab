import sys
import csv
from matplotlib import pyplot as plt

filename = 'insDependDist.csv'

MAX_DISPLAY = int(sys.argv[1]) if len(sys.argv) == 2 else 45

with open(filename) as f:
	reader = csv.reader(f)
	depend_row = next(reader)
	
	allDepend = 0
	for i in range(MAX_DISPLAY):
		allDepend += int(depend_row[i])
	
	percent = []
	for i in range(MAX_DISPLAY):
		percent.append(int(depend_row[i]) / allDepend)

fig = plt.figure(dpi = 100, figsize = (7, 4))
plt.plot([n for n in range(1, MAX_DISPLAY + 1)], percent[0:MAX_DISPLAY], c = 'red', marker = '.')

plt.title("Instuction Dependance Distance Distribution", fontsize = 16)
plt.xlabel('Dependence Distance', fontsize = 12)
plt.ylabel('Percent of Dependence', fontsize = 12)

plt.xlim(0, MAX_DISPLAY)
plt.ylim(0, percent[0] * 1.05)

plt.gca().xaxis.set_major_locator(plt.MultipleLocator(5))
plt.grid(linestyle = ':')
plt.show()

