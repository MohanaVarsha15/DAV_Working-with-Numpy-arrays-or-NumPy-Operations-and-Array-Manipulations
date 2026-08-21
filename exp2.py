import numpy as np
# Check NumPy version
print("NumPy Version:", np. version )
# Creating different types of arrays
arr_1d = np.array([1, 2, 3, 4, 5])
arr_2d = np.array([[1, 2, 3], [4, 5, 6]])
arr_0d = np.array(42)
arr_ones = np.ones((3, 3))
# Indexing and Slicing
print("Element at index 2 in 1D array:", arr_1d[2])
print("Element at row 1, column 2 in 2D array:", arr_2d[1, 2])
print("Slice from 1D array:", arr_1d[1:4])
print("Slice row 1 from 2D array:", arr_2d[1, :])
# Element-wise operations
arr_a = np.array([10, 20, 30])
arr_b = np.array([1, 2, 3])
print("Addition:", arr_a + arr_b)
print("Subtraction:", arr_a - arr_b)
print("Multiplication:", arr_a * arr_b)
print("Division:", arr_a / arr_b)
print("Scalar Multiplication:", arr_a * 2)
# Aggregations
print("Sum:", np.sum(arr_a))
print("Mean:", np.mean(arr_a))
print("Standard Deviation:", np.std(arr_a))
# Element-wise comparison
print("Element-wise comparison:", arr_a > arr_b)
# Boolean masking
print("Elements greater than 15:", arr_a[arr_a > 15])
# Fancy Indexing

indices = [0, 2]
print("Selected elements:", arr_a[indices])

# Reshape
reshaped_arr = arr_1d.reshape(5, 1)
print("Reshaped 1D array to 2D:\n", reshaped_arr)
# Structured array
structured_arr = np.array([(25, 90.5), (30, 85.2)], dtype=[('age', 'i4'), ('score', 'f4')])
print("Structured array:", structured_arr)