seq = []
for i in range(130):
    seq.append(440 * 2 ** ((i - 57) / 12))

for i in range(0, 132, 4):
    print(",".join([str(v) for v in seq[i : i + 4]]) + ",")
