from matplotlib import pyplot as plt
import os

prefix = 'brchPredict-'
suffix = '.txt'


def parse_one(name: str):
    with open(f"{prefix}{name}{suffix}") as f:
        lines = f.readlines()
        tests = []
        precisions = []
        for line in lines:
            if 'name' in line:
                tests.append(line[line.find(":") + 1:].strip())
            elif line.startswith("Precision:"):
                precisions.append(float(line.split(" ")[-1]))
        if len(tests) == 0:
            return None
        print(name, list(zip(tests, precisions)))
        return tests, precisions


def draw_ghr(data: dict):
    tests = list(data.keys())
    l = len(data[tests[0]][0])

    # result[7] name: GlobalHistoryPredictor<HashMethods::fold_xor<22>>(22, 17, 2, false)
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


def run():
    files = [f for f in os.listdir() if f.startswith(prefix) and f.endswith(suffix)]
    test_names = [f[len(prefix):-len(suffix)] for f in files]
    print("test names:", test_names)
    data = {}
    for name in test_names:
        data[name] = parse_one(name)
    draw_ghr(data)


if __name__ == '__main__':
    run()
