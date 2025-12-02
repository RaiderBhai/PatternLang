func add(int x, int y) {
    return x + y;
}
func inc(int v) {
    return add(v, 1);
}
func twice(int v) {
    return inc(inc(v));
}
print twice(5);
newline;
