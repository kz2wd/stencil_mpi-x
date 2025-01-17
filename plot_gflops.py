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

for halo_size, group in data.groupby("halo_size"):
    plt.plot(
        group["stencil_size"],
        group["gflops"],
        marker="o",
        label=f"halo_size={halo_size}",
    )

plt.xlabel("Stencil Size")
plt.ylabel("GFlops")
plt.title("Performance by Halo Size")
plt.xticks(rotation=45)
plt.legend(title="Halo Size")
plt.grid()

# current_date = datetime.now().strftime("%Y-%m-%d")

plt.tight_layout()

file_name = os.path.basename(file_path)
plot_name = f"gflops_on_size__{file_name}.png"
path = f"{plots_dir}/{plot_name}"
plt.savefig(path)
