from pathlib import Path
import re
from linq import QueryMethodBase


if __name__ == "__main__":
    FILE_PATH = "sample.txt"

    with Path(FILE_PATH).open(mode="r", encoding="utf-8") as f:
        result = QueryMethodBase(
            f
        ).select(
            lambda x: re.split(r"\s", x.strip())   # Split by spaces.
        ).flatten().groupby(
            lambda x: x
        ).select(
            lambda x: (x[0], sum(1 for _ in x[1]))
        ).orderby(
            lambda x: -x[1]
        ).to_list()
        print(result)