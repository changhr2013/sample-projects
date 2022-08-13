float ac[50], bc[50];

inline int multiply(int a, int b) {
    return a * b;
}

using namespace std;

/* 选择排序 */
void selectionSort(int arr[], int n) {

	for (int i = 0; i < n; i++)
	{
		// 寻找 [i, n) 区间里的最小值
		int minIndex = i;

		for (int j = i +1 ; j < n; j++)
		{
			if (arr[j] < arr[minIndex])
			{
				minIndex = j;
			}
		}
		swap(arr[i], arr[minIndex]);
	}
}

void hello(void) {
    printf("Hello, World!\n");
}