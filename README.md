# a http server to response client's IP
## build
```
export OMP_NUM_THREADS=16 # optionally
gcc -fopenmp  src/main.c  -o what-is-my-ip
```
