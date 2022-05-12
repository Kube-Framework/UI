
VectorDetails -> Resize must delete before init
SmallVector -> reserve directly optimized size to avoid preallocating (capacity *= 2) more than this size