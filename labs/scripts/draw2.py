from matplotlib import pyplot as plt
import os
from typing import *
from operator import itemgetter

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
            model_method = model[model.find(
                '-') + 1:] if '-' in model else None
            miss_rate = float(miss_rate[:-1])
            size = float(size.split()[0])
            data.append([model_name, model_args,
                        model_method, miss_rate, size])
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


def merge_data(data: dict, sort_data: bool = True) -> List:
    result: List = None
    if sort_data:
        for key in data:
            d = list(data[key])
            d = sorted(d, key=itemgetter(3, 2, 1))
            # print('\n'.join([str(x) for x in d]))
            data[key] = d
    # print(data)
    for test in data:
        if result is None:
            result = data[test]
        else:
            # print("result:", result)
            # print("data:", data[test])
            for i in range(len(result)):
                assert result[i][:3] == data[test][i][:
                                                      3], f"test={test}, i={i}, {result[i][:3]} != {data[test][i][:3]}"
                # print(result[i])
                # print(result[i][3], data[test][i][3])
                result[i] = [*result[i][:3], result[i]
                             [3] + data[test][i][3], result[i][4]]
                # result[i][3] = 0# = result[i][3] + data[test][i][3]
    for i in range(len(result)):
        result[i][3] /= len(result)
    return result


def draw_capacity(data: dict, groups: List[str] = ['FullAssoCache', 'DirectMappingCache', 'SetAsso_VIVT']):
    data_groups = {g: [d[1:] for d in data if g in d[0]] for g in groups}
    plots = []
    fig, ax = plt.subplots()
    ax.set_title("Capacity")
    ax.set_xlabel("capacity / KiB")
    ax.set_ylabel("miss rate / %")
    for i in range(len(groups)):
        group = groups[i]
        x = [v[3] for v in data_groups[group]]
        y = [v[2] for v in data_groups[group]]
        label = group
        if 'SetAsso' in group:
            (args, method) = data_groups[group][0][:2]
            label = group + f'''({", ".join(['n', *[str(a) for a in args[1:]]])})''' + (
                ('-' + method) if method is not None else "")
        line, = ax.plot(x, y, label=label)
        plots.append(line)
    ax.legend(handles=plots)
    plt.savefig(data_dir + "capacity.png")


def draw_asso(data: dict, groups: List[str] = ['SetAsso_VIVT', ]):
    data_groups = {g: [d[1:] for d in data if g in d[0]] for g in groups}
    plots = []
    fig, ax = plt.subplots()
    ax.set_title("asso")
    ax.set_xlabel("asso")
    ax.set_ylabel("miss rate / %")
    for i in range(len(groups)):
        group = groups[i]
        # print(data_groups[group])
        x = [v[0][2] for v in data_groups[group]]
        y = [v[2] for v in data_groups[group]]
        label = group
        if 'SetAsso' in group:
            (args, method) = data_groups[group][0][:2]
            label = group + f'''({", ".join([*[str(a) for a in args[:-1]], 'asso'])})''' + (
                ('-' + method) if method is not None else "")
        line, = ax.plot(x, y, label=label)
        plots.append(line)
    ax.legend(handles=plots)
    plt.savefig(data_dir + "asso.png")


def draw_block(data: dict, groups: List[str] = ['SetAsso_VIVT', ]):
    data_groups = {g: [d[1:] for d in data if g in d[0]] for g in groups}
    plots = []
    fig, ax = plt.subplots()
    ax.set_title("block")
    ax.set_xlabel("$2^{block}$")
    ax.set_ylabel("miss rate / %")
    for i in range(len(groups)):
        group = groups[i]
        # print(data_groups[group])
        x = [v[0][1] for v in data_groups[group]]
        y = [v[2] for v in data_groups[group]]
        label = group
        if 'SetAsso' in group:
            x = [2**xi for xi in x]
            (args, method) = data_groups[group][0][:2]
            label = group + f'''({", ".join([str(args[0]), 'block', str(args[2])])})''' + (
                ('-' + method) if method is not None else "")
        line, = ax.plot(x, y, label=label)
        plots.append(line)
    ax.legend(handles=plots)
    plt.savefig(data_dir + "block.png")


def load(pathname: str = '.', include_coremark: bool = True):
    files = [f for f in os.listdir(data_dir + pathname) if f.startswith(prefix) and
             f.endswith(suffix) and
             (('coremark' not in f) if not include_coremark else True)]
    test_names = [f[len(prefix):-len(suffix)] for f in files]
    print(f"dir: {pathname} test names: {test_names}")
    data = {}
    for name in test_names:
        d = parse_one(data_dir + pathname, name)
        if len(d) > 0:
            data[name] = d
    # draw_ghr(data)
    data = merge_data(data, sort_data='PV' != pathname)
    # print(pathname, data)
    if pathname == 'CAPACITY':
        draw_capacity(data)
    elif pathname == 'ASSO':
        draw_asso(data)
    elif pathname == 'BLOCK':
        draw_block(data)


def run():
    cls = os.listdir(data_dir)
    for c in cls:
        # if 'PV' == c:
        #     continue
        if '.' in c:
            continue
        load(c, include_coremark=False)


if __name__ == '__main__':
    # load()
    run()
