import pandas as pd
import ast
from scipy import stats
import numpy as np

"""
Script used to do the statistical analysis of the mfs and qt features.
The script uses the datas of the stats_all.txt and packet_count_all.txt files to do the analysis
"""

def load_file(path):
    df = pd.read_csv(path, sep=";")

    scenario_cols = df["scenario"].apply(ast.literal_eval).apply(pd.Series)
    scenario_cols.columns = [
        "nnodes", "topology", "nd", "datarate", "qt", "mfs"
    ]
    df = pd.concat([scenario_cols, df.drop(columns=["scenario"])], axis=1)

    return df

def mfs_groups(df):

    groups = {
        "NOMFS": df[df["mfs"] == "NO MFS"],
        "MFS5":    df[df["mfs"] == "MFS5"],
        # "MFS10":   df[df["mfs"] == "MFS10"],
        # "MFS30":   df[df["mfs"] == "MFS30"],
    }

    return groups

def qt_groups(df):

    groups = {
        "QT": df[df["qt"] == "EN"],
        "DEFAULT": df[df["qt"] == "DIS"],
    }

    return groups

def descriptive_statistics(groups, metrics):
    rows = []
    for group_name, df in groups.items():
        for metric in metrics:
            if metric not in df.columns: 
                continue

            data = df[metric].dropna()
            n = len(data)
            if n == 0:
                continue
            mean = np.mean(data)
            std = np.std(data, ddof=1)

            # intervalo de confiança 95%
            t_crit = stats.t.ppf(0.975, df=n-1)
            margin = t_crit * (std / np.sqrt(n))
            ci_low = mean - margin
            ci_high = mean + margin

            rows.append({
                "group": group_name,
                "metric": metric,
                "n": n,
                "mean": mean,
                "std": std,
                "ci_low": ci_low,
                "ci_high": ci_high
            })

    desc_table = pd.DataFrame(rows)
    final_table = (desc_table
                   .assign(val=lambda d: d["mean"].map("{:.3f}".format) + " ± " + d["std"].map("{:.3f}".format))
                   .pivot(index="metric", columns="group", values="val")
                   )
    #print(final_table)
    return final_table

metric_direction = {
    "delivery_data": True,
    "delivery_ctrl": True,
    "delay_data": False,
    "delay_ctrl": False,
    "ctrl_overhead": False,
    "energy": False,
    "fg_time": False,
}

stats_metrics = [
    "delivery_data",
    "delivery_ctrl",
    "delay_data",
    "delay_ctrl",
    "ctrl_overhead",
    "fg_time",
    "energy"
]

packets_types = [
    "Data flow request",
    "Neighbor report",
    "Data packet",
    "Neighbor discovery",
    "SDN_PACKET_CD",
    "Node ACK",
    "Controller ACK",
    "Register flowId",
    "Flow setup",
    "SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP"
]

# ============= MFS ==================

def cohens_d(a, b):
    a = np.array(a)
    b = np.array(b)

    na, nb = len(a), len(b)
    if na < 2 or nb < 2:
        return np.nan

    sa, sb = np.var(a, ddof=1), np.var(b, ddof=1)
    pooled_std = np.sqrt(((na - 1) * sa + (nb - 1) * sb) / (na + nb - 2))

    if pooled_std == 0:
        return 0.0

    return (np.mean(a) - np.mean(b)) / pooled_std

def interpret_d(d):
    d = abs(d)
    if d < 0.2:
        return "negligible"
    elif d < 0.5:
        return "small"
    elif d < 0.8:
        return "medium"
    else:
        return "large"

def run_mfs_statistical_tests(groups, metrics, output_path="report_mfs.txt"):
    with open(output_path, "a") as f:
        f.write("=" * 60 + "\n")
        f.write("STATISTICAL ANALYSIS\n")
        f.write("=" * 60 + "\n\n")

        for metric in metrics:
            f.write("-" * 25 + "\n")
            f.write(f"| METRIC: {metric} |\n")
            f.write("-" * 25 + "\n")

            # ANOVA
            samples = [g[metric].dropna().values for g in groups.values()]

            if all(len(s) > 1 for s in samples):
                F, p_anova = stats.f_oneway(*samples)
                f.write(f"ANOVA: F = {F:.4f}, p = {p_anova:.6g}\n")

                if p_anova < 0.05:
                    f.write("→ Significant difference between at least one MFS configuration.\n")
                else:
                    f.write("→ No significant difference detected.\n")
            else:
                f.write("ANOVA: insufficient data.\n")
                p_anova = np.nan

            f.write("\n")

            # Comparações vs NOMFS
            if "NOMFS" not in groups:
                f.write("Baseline NOMFS not found.\n\n")
                continue

            base = groups["NOMFS"][metric].dropna().values

            for name, g in groups.items():
                if name == "NOMFS":
                    continue

                sample = g[metric].dropna().values

                if len(sample) < 2 or len(base) < 2:
                    continue

                t, p = stats.ttest_ind(base, sample, equal_var=False)
                d = cohens_d(base, sample)
                size = interpret_d(d)

                sample_mean = np.mean(sample)
                base_mean = np.mean(base)

                increased = sample_mean > base_mean
                higher_better = metric_direction.get(metric, True)

                if increased:
                    if higher_better: final = "BETTER"
                    else: final = "WORSE"
                else:
                    if higher_better: final = "WORSE"
                    else: final = "BETTER"

                f.write(
                    f"{name} vs NOMFS → "
                    f"mean_nomfs = {base_mean:.4f}, "
                    f"mean_{name} = {sample_mean:.4f}, "
                    f"t = {t:.4f}, p = {p:.6g}, "
                    f"Cohen's d = {d:.3f} ({size}) → {final}\n"
                )
            f.write("\n")
            #f.write("\n" + "=" * 60 + "\n\n")

    print(f"Report saved to: {output_path}")

def run_compare_mfs_packets(groups, output_path="report_mfs.txt"):
    if "NOMFS" not in groups:
        print("NOMFS group not found.")
        return

    nomfs_flow = groups["NOMFS"]["Flow setup"].dropna().values

    with open(output_path, "a") as f:
        f.write("=" * 60 + "\n")
        f.write("MFS vs NOMFS – FLOW SETUP PACKET COMPARISON\n")
        f.write("=" * 60 + "\n\n")

        for mfs_group in ["MFS5", "MFS10"]:

            if mfs_group not in groups:
                continue

            mfs_packets = groups[mfs_group]["SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP"].dropna().values

            if len(mfs_packets) < 2 or len(nomfs_flow) < 2:
                continue

            mean_nomfs = np.mean(nomfs_flow)
            mean_mfs = np.mean(mfs_packets)

            t, p = stats.ttest_ind(nomfs_flow, mfs_packets, equal_var=False)
            d = cohens_d(nomfs_flow, mfs_packets)
            size = interpret_d(d)

            reduction = (mean_nomfs - mean_mfs) / mean_nomfs * 100

            if p < 0.05:
                if mean_mfs < mean_nomfs:
                    verdict = "SIGNIFICANT REDUCTION"
                else:
                    verdict = "SIGNIFICANT INCREASE"
            else:
                verdict = "NO SIGNIFICANT DIFFERENCE"

            f.write(
                f"{mfs_group} vs NOMFS →\n"
                f"Mean NOMFS FlowSetup = {mean_nomfs:.3f}\n"
                f"Mean {mfs_group} MultipleSetup = {mean_mfs:.3f}\n"
                f"Reduction = {reduction:.2f}%\n"
                f"t = {t:.4f}, p = {p:.6g}\n"
                f"Cohen's d = {d:.3f} ({size})\n"
                f"Conclusion: {verdict}\n\n"
                + "-" * 60 + "\n\n"
            )

mfs_stats_df = load_file("/home/lion/ic/itsdn/it-sdn-contiki-ng/simulation/TESTE/mfs_fix/mfs-res/stats/stats_all.txt")
mfs_packet_df = load_file("/home/lion/ic/itsdn/it-sdn-contiki-ng/simulation/TESTE/mfs_fix/mfs-res/stats/packet_count_all.txt")

mfs_stats_groups = mfs_groups(mfs_stats_df)
desc_table = descriptive_statistics(mfs_stats_groups, stats_metrics)

mfs_pkt_groups = mfs_groups(mfs_packet_df)
desc_table_pkt = descriptive_statistics(mfs_pkt_groups, packets_types)

output_file = "report_mfs.txt"
with open(output_file, "w") as f:
    f.write("=" * 60 + "\n")
    f.write("MFS OVERVIEW\n")
    f.write("=" * 60 + "\n\n")

    f.write(desc_table.to_string())
    f.write("\n\n")
    f.write(desc_table_pkt.to_string())
    f.write("\n\n")
    
run_mfs_statistical_tests(mfs_stats_groups, stats_metrics, output_file)
run_compare_mfs_packets(mfs_pkt_groups, output_file)


# ================================ QT ===================================
def run_qt_statistical_tests(groups, metrics, output_path="report_mfs.txt"):
    with open(output_path, "a") as f:
        f.write("=" * 60 + "\n")
        f.write("STATISTICAL ANALYSIS\n")
        f.write("=" * 60 + "\n\n")

        for metric in metrics:
            f.write("-" * 25 + "\n")
            f.write(f"| METRIC: {metric} |\n")
            f.write("-" * 25 + "\n")

            # ANOVA
            samples = [g[metric].dropna().values for g in groups.values()]

            if all(len(s) > 1 for s in samples):
                F, p_anova = stats.f_oneway(*samples)
                f.write(f"ANOVA: F = {F:.4f}, p = {p_anova:.6g}\n")

                if p_anova < 0.05:
                    f.write("→ Significant difference between QT configuration.\n")
                else:
                    f.write("→ No significant difference detected.\n")
            else:
                f.write("ANOVA: insufficient data.\n")
                p_anova = np.nan

            f.write("\n")

            # Comparações vs NOMFS
            if "DEFAULT" not in groups:
                f.write("Baseline QTDIS not found.\n\n")
                continue

            base = groups["DEFAULT"][metric].dropna().values

            for name, g in groups.items():
                if name == "DEFAULT":
                    continue

                sample = g[metric].dropna().values

                if len(sample) < 2 or len(base) < 2:
                    continue

                t, p = stats.ttest_ind(base, sample, equal_var=False)
                d = cohens_d(base, sample)
                size = interpret_d(d)

                sample_mean = np.mean(sample)
                base_mean = np.mean(base)

                increased = sample_mean > base_mean
                higher_better = metric_direction.get(metric, True)

                if increased:
                    if higher_better: final = "BETTER"
                    else: final = "WORSE"
                else:
                    if higher_better: final = "WORSE"
                    else: final = "BETTER"

                f.write(
                    f"QTEN vs DEFAULT → "
                    f"mean_default = {base_mean:.4f}, "
                    f"mean_qten = {sample_mean:.4f}, "
                    f"t = {t:.4f}, p = {p:.6g}, "
                    f"Cohen's d = {d:.3f} ({size}) → {final}\n"
                )
            f.write("\n")
            #f.write("\n" + "=" * 60 + "\n\n")

    print(f"Report saved to: {output_path}")

qt_stats_df = load_file("/home/lion/ic/itsdn/it-sdn-contiki-ng/simulation/qt/resultados/srcrtd/stats_all.txt")
qt_packet_df = load_file("/home/lion/ic/itsdn/it-sdn-contiki-ng/simulation/qt/resultados/srcrtd/packet_count_all.txt")

qt_stats_groups = qt_groups(qt_stats_df)
desc_table = descriptive_statistics(qt_stats_groups, stats_metrics)

qt_pkt_groups = qt_groups(qt_packet_df)
desc_table_pkt = descriptive_statistics(qt_pkt_groups, packets_types)

output_file = "report_qt.txt"
with open(output_file, "w") as f:
    f.write("=" * 60 + "\n")
    f.write("QUEUE TREATMENT OVERVIEW\n")
    f.write("=" * 60 + "\n\n")

    f.write(desc_table.to_string())
    f.write("\n\n")
    f.write(desc_table_pkt.to_string())
    f.write("\n\n")
    
run_qt_statistical_tests(qt_stats_groups, stats_metrics, output_file)

# ========================================================================