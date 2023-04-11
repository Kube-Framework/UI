
VectorDetails -> Resize must delete before init
SmallVector -> reserve directly optimized size to avoid preallocating (capacity *= 2) more than this size


# Executor & graphs
Focus pipelining at cost of latency in case of overcrowding of the scheduler
It means rearranging data to execute faster in sequential order