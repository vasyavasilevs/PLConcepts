from __future__ import annotations

import itertools
import typing

T = typing.TypeVar("T")

class QueryMethodBase:
    def __init__(self, iterable: typing.Iterable[T]) -> None:
        self._iterable: typing.Iterable[T] = iterable

    def select(self, selector_func: typing.Callable) -> QueryMethodBase:
        return QueryMethodBase(map(selector_func, self._iterable))

    def flatten(self) -> QueryMethodBase:
        def gen(iterable):
            flow = iterable
            for seq in flow:
                for element in seq:
                    yield element
        return QueryMethodBase(gen(self._iterable))

    def where(self, filter_func: typing.Callable) -> QueryMethodBase:
        return QueryMethodBase(filter(filter_func, self._iterable))

    def take(self, idx_to_take) -> QueryMethodBase:
        def gen(iterable):
            for idx, element in enumerate(iterable):
                if idx == idx_to_take:
                    break
                yield element
        return QueryMethodBase(gen(self._iterable))

    def groupby(self, key) -> QueryMethodBase:
        # Elements must be sorted.
        return QueryMethodBase(itertools.groupby(self.orderby(key)._iterable, key=key))

    def orderby(self, key) -> QueryMethodBase:
        return QueryMethodBase(sorted(self._iterable, key=key))

    def to_list(self) -> typing.List[T]:
        return list(self._iterable)