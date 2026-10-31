"""Compara los resúmenes de las tres configuraciones del banco.

Uso:
    uv run comparar.py mediciones/..._config-a_resumen.csv mediciones/..._config-b_resumen.csv mediciones/..._config-c_resumen.csv

Saca la ganancia del ducto (b/a) y la pérdida de las rejillas (c/b) en cada paso, y los tres
números que van a gavia-hardware/cad/blender/gavia_aro.py.
"""

import csv
import sys

# Regla de decisión del banco (18_plan_de_prototipo en la wiki): con ducto, labio y rejillas,
# unos 1,8 kg o más por motor al 100 %.
CRITERIO_G = 1800


def leer(ruta):
    with open(ruta, encoding="utf-8") as f:
        return {int(r["acelerador_pct"]): float(r["empuje_medio_g"]) for r in csv.DictReader(f)}


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    a, b, c = (leer(ruta) for ruta in sys.argv[1:])

    print(" acel.        a        b        c     b/a     c/b")
    for pct in sorted(a):
        if pct not in b or pct not in c:
            continue
        print(f" {pct:4} %  {a[pct]:6.0f} g {b[pct]:6.0f} g {c[pct]:6.0f} g"
              f"   {b[pct] / a[pct]:5.2f}   {c[pct] / b[pct]:5.2f}")

    tope = max(set(a) & set(b) & set(c))
    print(f"\nPara gavia_aro.py (al {tope} %):")
    print(f"    \"kg_por_motor\": {a[tope] / 1000:.2f},")
    print(f"    \"k_ducto\": {b[tope] / a[tope]:.2f},")
    print(f"    \"k_rejilla\": {c[tope] / b[tope]:.2f},")
    if tope == 100:
        veredicto = "pasa" if c[tope] >= CRITERIO_G else "no pasa"
        print(f"\nCon ducto, labio y rejillas empuja {c[tope] / 1000:.2f} kg al 100 %: {veredicto} "
              f"el criterio de {CRITERIO_G / 1000:.1f} kg por motor.")
    else:
        print("\nEl barrido no llegó al 100 %, así que el criterio de 1,8 kg queda sin evaluar.")


if __name__ == "__main__":
    main()
