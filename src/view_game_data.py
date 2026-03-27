from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


METRICS = [
	"mean_score",
	"mean_max_tile",
	"median_score",
	"median_max_tile",
]


def find_csv_path(csv_path: str | None = None) -> Path:
	"""Find the grid search CSV path from explicit input or common filenames."""
	if csv_path:
		candidate = Path(csv_path).expanduser().resolve()
		if candidate.exists():
			return candidate
		raise FileNotFoundError(f"CSV introuvable: {candidate}")

	script_dir = Path(__file__).resolve().parent
	project_root = script_dir.parent

	candidates = [
		project_root / "grid_search_log_copy.csv",
		project_root / "grid_search_log copy.csv",
		project_root / "grid_search_log.csv",
	]

	for candidate in candidates:
		if candidate.exists():
			return candidate

	raise FileNotFoundError(
		"Impossible de trouver un fichier CSV de grid search. "
		"Noms essayes: grid_search_log_copy.csv, grid_search_log copy.csv, grid_search_log.csv"
	)


def validate_columns(df: pd.DataFrame) -> None:
	required_columns = {"gradient_weight", "merge_weight", *METRICS}
	missing = required_columns - set(df.columns)
	if missing:
		raise ValueError(f"Colonnes manquantes dans le CSV: {sorted(missing)}")


def plot_heatmaps(df: pd.DataFrame, output: str | None = None, show: bool = True) -> None:
	"""Plot 2x2 heatmaps for the main grid search metrics."""
	validate_columns(df)

	fig, axes = plt.subplots(2, 2, figsize=(16, 12), constrained_layout=True)
	fig.suptitle("Resultats Grid Search - IA 2048", fontsize=16, fontweight="bold")

	for ax, metric in zip(axes.flatten(), METRICS):
		pivot = df.pivot(index="gradient_weight", columns="merge_weight", values=metric)
		pivot = pivot.sort_index().sort_index(axis=1)

		im = ax.imshow(pivot.values, origin="lower", aspect="auto", cmap="viridis")
		ax.set_title(metric)
		ax.set_xlabel("merge_weight")
		ax.set_ylabel("gradient_weight")
		ax.set_xticks(range(len(pivot.columns)))
		ax.set_xticklabels([f"{x:g}" for x in pivot.columns], rotation=45, ha="right")
		ax.set_yticks(range(len(pivot.index)))
		ax.set_yticklabels([f"{y:g}" for y in pivot.index])
		fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

	# Highlight the best configuration by mean score.
	best_row = df.loc[df["mean_score"].idxmax()]
	print("Meilleure configuration (mean_score):")
	print(
		f"  gradient_weight={best_row['gradient_weight']}, "
		f"merge_weight={best_row['merge_weight']}, "
		f"mean_score={best_row['mean_score']}, "
		f"mean_max_tile={best_row['mean_max_tile']}"
	)

	if output:
		output_path = Path(output).expanduser().resolve()
		output_path.parent.mkdir(parents=True, exist_ok=True)
		fig.savefig(output_path, dpi=200)
		print(f"Figure enregistree: {output_path}")

	if show:
		plt.show()


def build_ratio_dataframe(
	df: pd.DataFrame,
	direction: str = "both",
	precision: int = 6,
) -> tuple[pd.DataFrame, int, int]:
	"""Group results by ratio in one or both directions."""
	validate_columns(df)
	work = df.copy()

	gw = work["gradient_weight"].astype(float)
	mw = work["merge_weight"].astype(float)
	undefined_count = int(((gw == 0) & (mw == 0)).sum())

	parts: list[pd.DataFrame] = []
	infinite_count = 0

	if direction in {"g_over_m", "both"}:
		g_over_m = work.copy()
		g_over_m["weight_ratio"] = np.where(mw == 0, np.nan, gw / mw)
		g_over_m["ratio_direction"] = "gradient/merge"
		infinite_count += int(((mw == 0) & (gw != 0)).sum())
		parts.append(g_over_m)

	if direction in {"m_over_g", "both"}:
		m_over_g = work.copy()
		m_over_g["weight_ratio"] = np.where(gw == 0, np.nan, mw / gw)
		m_over_g["ratio_direction"] = "merge/gradient"
		infinite_count += int(((gw == 0) & (mw != 0)).sum())
		parts.append(m_over_g)

	all_ratios = pd.concat(parts, ignore_index=True)

	# Keep only finite ratios for a meaningful x-axis curve.
	finite = all_ratios[np.isfinite(all_ratios["weight_ratio"])].copy()
	finite["weight_ratio"] = finite["weight_ratio"].round(precision)

	agg = (
		finite.groupby(["ratio_direction", "weight_ratio"], as_index=False)
		.agg(
			count=("weight_ratio", "size"),
			**{metric: (metric, "mean") for metric in METRICS},
		)
		.sort_values(["ratio_direction", "weight_ratio"])
	)

	return agg, undefined_count, infinite_count


def plot_ratio_curves(
	df: pd.DataFrame,
	output: str | None = None,
	show: bool = True,
	direction: str = "both",
	log_x: bool = True,
) -> None:
	"""Plot one curve per metric against ratio gradient_weight / merge_weight."""
	ratio_df, undefined_count, infinite_count = build_ratio_dataframe(df, direction=direction)
	if ratio_df.empty:
		raise ValueError("Aucune ligne avec un ratio fini n'a ete trouvee.")

	fig, axes = plt.subplots(2, 2, figsize=(16, 12), constrained_layout=True)
	fig.suptitle("Metriques en fonction des ratios de poids", fontsize=16, fontweight="bold")

	for ax, metric in zip(axes.flatten(), METRICS):
		for ratio_direction, group in ratio_df.groupby("ratio_direction"):
			ax.plot(
				group["weight_ratio"],
				group[metric],
				marker="o",
				linewidth=2,
				markersize=4,
				label=ratio_direction,
			)
		ax.set_title(metric)
		ax.set_xlabel("ratio de poids")
		ax.set_ylabel(metric)
		ax.grid(alpha=0.3)
		if log_x:
			ax.set_xscale("log")
			ax.axvline(1.0, color="gray", linestyle="--", linewidth=1)
		ax.legend()

	print("Regroupement par ratio termine.")
	print(f"  Ratios distincts (finis): {len(ratio_df)}")
	print(f"  Direction de ratio: {direction}")
	print(f"  Axe x logarithmique: {log_x}")
	print(f"  Lignes ignorees (0/0, ratio indefini): {undefined_count}")
	print(f"  Lignes ignorees (ratio infini): {infinite_count}")

	for ratio_direction, group in ratio_df.groupby("ratio_direction"):
		for metric in METRICS:
			best = group.loc[group[metric].idxmax()]
			print(
				f"  [{ratio_direction}] meilleur ratio pour {metric}: "
				f"ratio={best['weight_ratio']}, valeur={best[metric]:.3f}, n={int(best['count'])}"
			)

	if output:
		output_path = Path(output).expanduser().resolve()
		output_path.parent.mkdir(parents=True, exist_ok=True)
		fig.savefig(output_path, dpi=200)
		print(f"Figure enregistree: {output_path}")

	if show:
		plt.show()


def resolve_output_path(save: str | None, suffix: str) -> str | None:
	if not save:
		return None
	save_path = Path(save)
	if save_path.suffix:
		return str(save_path.with_name(f"{save_path.stem}_{suffix}{save_path.suffix}"))
	return str(save_path.with_name(f"{save_path.name}_{suffix}.png"))


def main() -> None:
	parser = argparse.ArgumentParser(
		description="Visualise les resultats de grid search du solveur 2048"
	)
	parser.add_argument(
		"--csv",
		type=str,
		default=None,
		help="Chemin vers le fichier CSV (optionnel)",
	)
	parser.add_argument(
		"--save",
		type=str,
		default=None,
		help="Chemin de sortie pour sauvegarder la figure (optionnel)",
	)
	parser.add_argument(
		"--no-show",
		action="store_true",
		help="N'affiche pas la figure (utile en execution headless)",
	)
	parser.add_argument(
		"--view",
		type=str,
		choices=["heatmap", "ratio", "both"],
		default="ratio",
		help="Type de visualisation a produire (defaut: ratio)",
	)
	parser.add_argument(
		"--ratio-direction",
		type=str,
		choices=["g_over_m", "m_over_g", "both"],
		default="g_over_m",
		help="Sens du ratio en mode ratio (defaut: g_over_m)",
	)
	parser.add_argument(
		"--linear-x",
		action="store_true",
		help="Utilise un axe x lineaire en mode ratio (defaut: logarithmique)",
	)
	args = parser.parse_args()

	csv_path = find_csv_path(args.csv)
	print(f"Chargement du CSV: {csv_path}")

	df = pd.read_csv(csv_path)
	if args.view == "heatmap":
		plot_heatmaps(df, output=args.save, show=not args.no_show)
	elif args.view == "ratio":
		plot_ratio_curves(
			df,
			output=args.save,
			show=not args.no_show,
			direction=args.ratio_direction,
			log_x=not args.linear_x,
		)
	else:
		heatmap_out = resolve_output_path(args.save, "heatmap")
		ratio_out = resolve_output_path(args.save, "ratio")
		plot_heatmaps(df, output=heatmap_out, show=not args.no_show)
		plot_ratio_curves(
			df,
			output=ratio_out,
			show=not args.no_show,
			direction=args.ratio_direction,
			log_x=not args.linear_x,
		)


if __name__ == "__main__":
	main()
