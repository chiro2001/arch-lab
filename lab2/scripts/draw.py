from matplotlib import pyplot as plt
import os


def run():
    files = [f for f in os.listdir() if 'brchPredict' in f and f.endswith('.txt')]
    test_names = [f[len('brchPredict-'), 2] for f in files]


if __name__ == '__main__':
    run()
