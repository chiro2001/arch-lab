from matplotlib import pyplot as plt
import os
from typing import *

prefix = 'cacheModels-'
suffix = '.txt'
data_dir = "data/cacheModel/"


def parse_one(base: str, name: str):
    with open(f"{base}/{prefix}{name}{suffix}") as f:
        lines = f.readlines()
        lines = lines[2:]
        data = []
        for line in lines:
            model, miss_rate, size = [
                s.strip() for s in line.split('|') if len(s.strip()) > 0]
            model_name = model[:model.find('(')]
            model_args = model[model.find('(') + 1:model.find(')')]
            model_args = [int(a) for a in model_args.split(", ")]
            model_method = model[model.find('-') + 1:] if '-' in model else None
            miss_rate = float(miss_rate[:-1])
            size = float(size.split()[0])
            data.append((model_name, model_args,
                        model_method, miss_rate, size))
        # print(name, data)
        return data


'''
|              model              |  miss rate  |    size   |
| ------------------------------- |  ---------  | --------- |
|           FullAssoCache(512, 6) | 0.00021362% | 35.69 KiB |
|           FullAssoCache(256, 6) | 0.00026703% | 17.84 KiB |
|      DirectMappingCache(512, 6) | 0.00027466% | 33.69 KiB |
|           FullAssoCache(128, 6) | 0.00034332% |  8.92 KiB |
|      DirectMappingCache(256, 6) | 0.00042725% | 16.84 KiB |
|      DirectMappingCache(128, 6) | 0.00077057% |  8.42 KiB |
'''


def draw_ghr(data: dict):
    tests = list(data.keys())
    l = len(data[tests[0]][0])

    def get_width(config: str) -> int:
        return int(config[config.find("(") + 1:-1].split(", ")[0])

    plots = []
    fig, ax = plt.subplots()
    for i in range(len(tests)):
        test = tests[i]
        x = [get_width(v) for v in data[test][0]]
        y = data[test][1]
        print(x, y)
        line, = ax.plot(x, y, label=test)
        plots.append(line)
    ax.legend(handles=plots)
    plt.show()

def merge_data(data: dict) -> List:
    result: List = None
    for test in data:
        if result is None:
            result = data[test]
        else:
            for i in range(len(result)):
                print(result[i], data[test][i])
    return result

def draw_data_group(data: dict, groups: List[str] = ['FullAssoCache', 'DirectMappingCache', 'SetAsso_VIVT']):
    pass


def load(pathname: str = '.', include_coremark: bool = True):
    files = [f for f in os.listdir(data_dir + pathname) if f.startswith(prefix) and
             f.endswith(suffix) and
             (('coremark' not in f) if not include_coremark else True)]
    test_names = [f[len(prefix):-len(suffix)] for f in files]
    print(f"dir: {pathname} test names: {test_names}")
    data = {}
    for name in test_names:
        data[name] = parse_one(data_dir + pathname, name)
    # print(data)
    # draw_ghr(data)
    data = merge_data(data)


def run():
    cls = os.listdir(data_dir)
    for c in cls:
        load(c, include_coremark=False)


if __name__ == '__main__':
    # load()
    run()
