import matplotlib
matplotlib.use('QtAgg') 
import matplotlib.pyplot as plt
import subprocess as subprocess
import numpy as np
import os

NB_MAX_THREADS = 2000
NB_MAX_SWITCHS = 3000

def evaluate_execution_time(nb_threads):
    result = subprocess.run(["./install/bin/21-create-many", str(nb_threads)], capture_output=True, text=True)
    return int(result.stdout.split(" ")[-2])
    
def get_execution_time(nb_max_threads):
    temps_execution = {}
    nb_threads = np.arange(0, nb_max_threads, 10)
    for n in nb_threads:
        temps_execution[n] = evaluate_execution_time(n)
    return temps_execution
   
   
#######################""
def evaluate_execution_time_pthread(nb_threads):
    result = subprocess.run(["./install/bin_pthread/21-create-many", str(nb_threads)], capture_output=True, text=True)
    return int(result.stdout.split(" ")[-2])
    
def get_execution_time_pthread(nb_max_threads):
    temps_execution = {}
    nb_threads = np.arange(0, nb_max_threads, 10)
    for n in nb_threads:
        temps_execution[n] = evaluate_execution_time_pthread(n)
    return temps_execution
       
     
  #######################################
def evaluate_switch_time (nb_threads, nb_switch):
    result = subprocess.run(["./install/bin/31-switch-many", str(nb_threads), str(nb_switch)], capture_output=True, text=True)
    return int(result.stdout.split(" ")[-2])
    
def evaluate_switch_time_pthread (nb_threads, nb_switch):
    result = subprocess.run(["./install/bin_pthread/31-switch-many", str(nb_threads), str(nb_switch)], capture_output=True, text=True)
    return int(result.stdout.split(" ")[-2])
    
def get_switch_time(nb_threads, nb_max_switch):
    temps_execution = {}
    nb_swicth = np.arange(0, nb_max_switch, 10)
    for n in nb_swicth:
        temps_execution[n] = evaluate_switch_time( nb_threads, n)
    return temps_execution
         
def get_switch_time_pthread(nb_threads, nb_max_switch):
    temps_execution = {}
    nb_swicth = np.arange(0, nb_max_switch, 10)
    for n in nb_swicth:
        temps_execution[n] = evaluate_switch_time_pthread( nb_threads, n)
    return temps_execution
   ###########################################""      
directory = "./graphs/figures"
if not os.path.exists(directory):
    os.makedirs(directory)

def plot_execution_time_graph():
    temps_execution = get_execution_time(NB_MAX_THREADS)
    execution_times_x = list(temps_execution.values())
    execution_times_y= list(temps_execution.keys())
    
    temps_execution_p = get_execution_time_pthread(NB_MAX_THREADS)
    execution_times_x_p = list(temps_execution_p.values())
    execution_times_y_p= list(temps_execution_p.keys())
    plt.plot(execution_times_y, execution_times_x)
    print(execution_times_y_p)
    plt.plot(execution_times_y_p, execution_times_x_p)
    plt.plot(execution_times_y, execution_times_x)
    plt.xlabel("nombre de threads")
    plt.ylabel("temps d'exécution en μs") 
    plt.savefig("./graphs/figures/Execution_time.png")
    plt.show()

def plot_switch_time_graph():
    temps_execution = get_switch_time(20, NB_MAX_SWITCHS)
    execution_times_x = list(temps_execution.values())
    execution_times_y = list(temps_execution.keys())
    
    temps_execution_p = get_switch_time_pthread(20, NB_MAX_THREADS)
    execution_times_x_p = list(temps_execution_p.values())
    execution_times_y_p= list(temps_execution_p.keys())
    plt.plot(execution_times_y_p, execution_times_x_p)
    plt.xlabel("nombre de switches")
    plt.ylabel("temps d'exécution en μs") 
    plt.savefig("./graphs/figures/Switches_time.png")
    plt.show()


if __name__ == "__main__":
    plot_execution_time_graph()
    plot_switch_time_graph()