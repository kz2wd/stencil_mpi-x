import pandas as pd
import matplotlib.pyplot as plt
import os

# from datetime import datetime
import sys

if len(sys.argv) > 1:
    file_path = sys.argv[1]
else:
    print("invalid arguments, do:")
    print("./plot_gflops.py file_path")
    exit

plots_dir = "plots"
data_dir = "data"


data = pd.read_csv(file_path)

unique_np_mpi = data["np_mpi"].unique()

n_subplots = len(unique_np_mpi)
fig, axes = plt.subplots(nrows=n_subplots, figsize=(10, 6 * n_subplots), sharex=True)

plt.xscale("log")

if n_subplots == 1:
    axes = [axes]

for ax, np_mpi in zip(axes, unique_np_mpi):
    subset = data[data["np_mpi"] == np_mpi]

    for halo_size, group in subset.groupby("halo_size"):
        ax.plot(
            group["stencil_size"],
            group["gflops"],
            marker="o",
            label=f"halo_size={halo_size}",
        )

    ax.set_title(f"Logarithmic performance for np_mpi={np_mpi}")
    ax.set_xlabel("Stencil Size")
    ax.set_ylabel("GFlops")
    ax.legend(title="Halo Size")
    ax.grid()
    ax.tick_params(axis="x", rotation=45)

plt.tight_layout()

# plt.show()


file_name = os.path.basename(file_path)
plot_name = f"gflops_on_size__{file_name}.png"
path = f"{plots_dir}/{plot_name}"
plt.savefig(path)
