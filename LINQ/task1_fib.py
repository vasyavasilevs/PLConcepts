from linq import QueryMethodBase


def fib():
    prevprev = 1
    prev = 1
    yield prevprev
    yield prev
    while True:
        next = prev + prevprev
        yield next
        prevprev = prev
        prev = next


if __name__ == "__main__":
    TAKE_NUMBERS_COUNT = 5
    result = QueryMethodBase(
        fib()
    ).where(
        lambda x: x % 3 == 0
    ).select(
        lambda x: x ** 2 if x % 2 == 0 else x
    ).take(TAKE_NUMBERS_COUNT).to_list()
    print(result)
