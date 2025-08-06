import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

def on_submit():
    try:
        nodes_str = entry_nodes.get()
        nodes_v = [int(x.strip()) for x in nodes_str.split(',') if x.strip()]
        min_iter = int(entry_min_iter.get())
        max_iter = int(entry_max_iter.get())

        selected_topologies = [top for top, var in topology_vars.items() if var.get()]
        selected_linktypes = [link for link, var in link_vars.items() if var.get()]
        selected_nd = [nd for nd, var in nd_vars.items() if var.get()]

        datarates_str = entry_datarates.get()
        datarates = [int(x.strip()) for x in datarates_str.split(',') if x.strip()]

        simulation_time = float(simulation_time_iter.get())

        fileprefix = entry_fileprefix.get() if entry_fileprefix.get() else ""

        print("NODES:", nodes_v)
        print("TOPOLOGIES:", selected_topologies)
        print("LINKS TYPES:", selected_linktypes)
        print("NEIGHBOR DISCOVERY:", selected_nd)
        print("MIN_ITER:", min_iter)
        print("MAX_ITER:", max_iter)
        print("DATARATE:", datarates)
        print("SIMULATION TIME:", simulation_time)
        print("FILEPREFIX:", fileprefix)

        messagebox.showinfo("Dados capturados", "As variáveis foram recebidas com sucesso!")

    except Exception as e:
        messagebox.showerror("Erro", f"Erro ao ler os dados: {e}")

##############
#    GUI     #
##############

root = tk.Tk()
root.title("Configuração do Experimento")
root.geometry("600x900")
#------------------
ttk.Label(root, text="Número de nós").pack(pady=5)
entry_nodes = ttk.Entry(root, width=50)
entry_nodes.pack()
entry_nodes.insert(0, "16, 25, 36, 49, 64, 81, 169")
#------------------
ttk.Label(root, text="Iteração inicial").pack(pady=5)
entry_min_iter = ttk.Entry(root, width=10)
entry_min_iter.pack()
entry_min_iter.insert(0, "1")
#------------------
ttk.Label(root, text="Iteração final").pack(pady=5)
entry_max_iter = ttk.Entry(root, width=10)
entry_max_iter.pack()
entry_max_iter.insert(0, "10")
#------------------
ttk.Label(root, text="Topologias:").pack(pady=5)
topology_frame = ttk.Frame(root)
topology_frame.pack()

topology_options = ["GRID", "BERLIN"]
topology_vars = {}
for topo in topology_options:
    var = tk.BooleanVar()
    cb = ttk.Checkbutton(topology_frame, text=topo, variable=var)
    cb.pack(anchor='w')
    topology_vars[topo] = var
#------------------
ttk.Label(root, text="Links:").pack(pady=5)
link_frame = ttk.Frame(root)
link_frame.pack()

link_options = ["FULL", "RDN", "CTA", "SPN"]
link_vars = {}
for link in link_options:
    var = tk.BooleanVar()
    cb = ttk.Checkbutton(link_frame, text=link, variable=var)
    cb.pack(anchor='w')
    link_vars[link] = var
#------------------
ttk.Label(root, text="Protocolos:").pack(pady=5)
newdir_frame = ttk.Frame(root)
newdir_frame.pack()

nd_options = ["IM-SC-nullrdc-truebidir", "OP-SIMPLE", "OP-FAST", "NEW-ALGO-V2"]
nd_vars = {}
for nd in nd_options:
    var = tk.BooleanVar()
    cb = ttk.Checkbutton(newdir_frame, text=nd, variable=var)
    cb.pack(anchor='w')
    nd_vars[nd] = var
#------------------
ttk.Label(root, text="Datarates").pack(pady=5)
entry_datarates = ttk.Entry(root, width=50)
entry_datarates.pack()
entry_datarates.insert(0, "1")
#------------------
ttk.Label(root, text="Fileprefix").pack(pady=5)
entry_fileprefix = ttk.Entry(root, width=50)
entry_fileprefix.pack()
#------------------
ttk.Label(root, text="Simulation time").pack(pady=5)
simulation_time_iter = ttk.Entry(root, width=10)
simulation_time_iter.pack()
simulation_time_iter.insert(0, "3600.0")
#------------------
ttk.Button(root, text="Confirmar", command=on_submit).pack(pady=20)

root.mainloop()
