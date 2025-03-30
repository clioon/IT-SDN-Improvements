import sys

num = sys.argv[1]
regex = "^("

for i in reversed(range(0,len(num))):
	digit = int(num[i])
	print("index", i, "digit:", digit,end="\t")

	if digit == 9:
		print("\tdigit is 9, skipping")
		continue
	elif digit == 8:
		exp = num[0:i] + "9"
	else:
		exp = num[0:i] + "[" + repr(digit + 1) + "-9]"

	trailing_digits_len = len(num) - i - 1
	if trailing_digits_len >= 1:
		exp +=  "[0-9]"
	if trailing_digits_len > 1:
		exp +=  "{" + repr(trailing_digits_len) + "}"


	print("\texp =", exp)

	regex += "(" + exp + ")|"

regex += "([0-9]{" + repr(len(num) + 1) + ",})"
regex += "):"

print(regex)


