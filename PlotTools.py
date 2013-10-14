import numpy as np
import matplotlib.pyplot as plt
import glob
import os
import re

def load_results(file_name):
    run_number = 0
    results = []
    while True:
        try:
            results.append(np.loadtxt(file_name.format(run_number)))
            run_number += 1
        except IOError:
            break
    print("Loaded {} runs".format(run_number))
    return np.array(results)

def load_directory(directory):
    for pathname in glob.iglob (os.path.join(directory, "*-0.txt")):
        pattern = re.sub (r'^(.*-)0(\.txt)$', r'\1{}\2', pathname)
        (basename,) = re.findall (r'^(.*)-0.txt', os.path.basename(pathname), flags=re.I)
        print ("Loading {}".format(basename))
        yield (basename, load_results(pattern))

def aggregate_results(results):
    nresults = np.size(results, axis=0)
    results_mean = results.mean(axis=0)
    results_stderr = results.std(axis=0) / np.sqrt(float(nresults))
    cumresults_stderr = results.cumsum(axis=1).std(axis=0) / np.sqrt(float(nresults))
    return (results_mean, results_stderr, cumresults_stderr)

def summarize_directory (directory, output_directory=None):
    if output_directory is None: output_directory = directory
    for (name, results) in load_directory(directory):
        np.savetxt (os.path.join(output_directory, '{}.txt'.format(name)),
                    np.array(aggregate_results(results)).T,
                    header='{} ({} runs)'.format(name, np.size(results, axis=0)))

def plot_results(mean, stderr, label,  above=False):
    plt.fill_between(np.arange(mean.size), mean+2*stderr, mean-2*stderr,
                     color='moccasin', edgecolor='moccasin')
    plt.plot(mean, color='black', lw=2)

    xposition = mean.size - 20
    if above:
        yposition = mean[xposition] + 2.5*stderr[xposition]
        va = 'bottom'
    else:
        yposition = mean[xposition] - 2.5*stderr[xposition]
        va = 'top'
    plt.text(xposition, yposition, label, va=va, ha='right', size=10, family='serif')

def set_labels(xtext, ytext):
    plt.xlabel(xtext, size=10, family='serif', ha='right', x=1)
    plt.ylabel(ytext, size=10, rotation='horizontal', multialignment='right',
               ha='right', va='top', y=1.0, family='serif')

def set_tick_style():
    plt.xticks(size=9, family="serif")
    plt.yticks(size=9, family="serif")
