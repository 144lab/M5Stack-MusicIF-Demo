seq = []
for i in range(128):
    seq.append(440 * 2 ** ((i - 69) / 12))

for i in range(0, 128, 4):
    print(",".join([str(1.0 / v) for v in seq[i : i + 4]]))
