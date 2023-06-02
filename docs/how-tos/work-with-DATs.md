# How to work with DATs

Working with DATs in 1.2.0 is not quite on-par with working with CHOPs and this is an area where we want to improve in a future update.

At the moment, the nodes available to work with DATs are as bellow.

- Get Cell: returns FString of value at index row, col
- Get Cell by Name: same as get cell, but uses the first cell of the row and column as their names
- Get Row: returns a FString array of the row at index row
- Get Row by Name: same as get row, but uses the first cell of the row as its name
- Get Column: returns a FString array of the column at index col
- Get Column by Name: same as get column, but uses the first cell of the column as its name

When writing data or preparing data for a DAT Input, users can pass arrays of strings. Internally, this will result in creating a column.