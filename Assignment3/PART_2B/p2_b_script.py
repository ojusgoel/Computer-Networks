import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import subprocess

rng_runs = range(90, 100)
error_rates = [0.2, 0.4, 0.6, 0.8, 1.0]
errorUnits = ["bit", "byte", "packet"]

for errorUnit in errorUnits:

    pdr_values = []

    # Collecting PDR values for each combination of RngRun and ErrorRate
    for rng_run in rng_runs:
        for error_rate in error_rates:
            command = f'./ns3 run "scratch/p2_b.cc --RngRun={rng_run} --error={error_rate} --errorUnit={errorUnit}"'
            output = subprocess.check_output(command, shell=True).decode("utf-8")
            last_line = output.strip().split("\n")[-1]
            pdr = float(last_line.split(": ")[-1])
            pdr_round = round(pdr, 3)
            pdr_values.append((rng_run, error_rate, pdr_round))

    # Creating DataFrame with PDR values
    df = pd.DataFrame(pdr_values, columns=["RngRun", "ErrorRate", "PDR"])
    df_pivot = df.pivot(index="RngRun", columns="ErrorRate", values="PDR")

    # Calculate the average PDR for each ErrorRate and append it as a new row
    mean_pdr = df_pivot.mean()
    df_pivot.loc['Average'] = mean_pdr

    print(df_pivot)

    # Display the table with the average row included
    fig, ax = plt.subplots(figsize=(12, 8))
    ax.axis('tight')
    ax.axis('off')

    # Creating table with PDR values and the average row
    table = ax.table(cellText=np.round(df_pivot.values, 3),
                     colLabels=df_pivot.columns,
                     rowLabels=df_pivot.index,
                     cellLoc='center',
                     loc='center')

    plt.title(f'PDR Matrix for {errorUnit} Error Unit')
    plt.tight_layout()
    plt.show()

    # Plot Average PDR vs Error Rate
    x_ticks = np.arange(0.2, 1.2, 0.2)
    plt.figure(figsize=(10, 8))
    plt.plot(error_rates, mean_pdr, marker='o', color='orange')
    plt.xlabel('Error Rate')
    plt.ylabel('Average PDR')
    plt.title(f'Average PDR vs Error Rate for {errorUnit} Error Unit')
    plt.xticks(x_ticks)  
    plt.tight_layout()
    plt.show()
