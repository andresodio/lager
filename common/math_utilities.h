/*
 * Based on code from:
 * http://stackoverflow.com/questions/4229870/c-algorithm-to-calculate-least-common-multiple-for-multiple-numbers/4229930#4229930
 */
int GreatestCommonDenominator(int a, int b)
{
	for (;;)
	{
		if (a == 0) return b;
		b %= a;
		if (b == 0) return a;
		a %= b;
	}
}

/*
 * Based on code from:
 * http://stackoverflow.com/questions/4229870/c-algorithm-to-calculate-least-common-multiple-for-multiple-numbers/4229930#4229930
 */
int LeastCommonMultiple(int a, int b)
{
	int temp = GreatestCommonDenominator(a, b);

	return temp ? (a / temp * b) : 0;
}
