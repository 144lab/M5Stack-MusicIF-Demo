import math

SampleRate = 8000
f = 440
N = 128
form = []
for i in range(N):
    form.append(math.sin(2 * math.pi * i / N))

print(form)
phase = 0.0
delta = 1.0 / SampleRate


def op(f):
    global phase
    i = int(float(N) * phase)
    phase = phase - math.trunc(phase)
    v = form[i % N] * (1 - phase) + form[(i + 1) % N] * (phase)
    print("{0:+5.3f}/".format(phase), end="")
    phase += f / float(SampleRate)
    return v


for i in range(30):
    print("{0:4d}:{1:+5.3f}".format(i, op(440)))
