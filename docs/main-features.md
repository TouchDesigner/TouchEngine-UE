# Main features

![assets/main-features-nodes01.png?raw=true](assets/main-features-nodes01.png?raw=true)
![assets/main-features-nodes02.png?raw=true](assets/main-features-nodes02.png?raw=true)

### General

- Set TouchEngine Input: sets the value of an input parameter
- Get TouchEngine Input Current Value: gets the current value of an input parameter
- Get TouchEngine Output: gets the value of an output parameter
- IsRunning: returns whether or not the instance is currently running.
- Start TouchEngine: creates the TE Instance, will call ToxLoaded or ToxFailedLoad callbacks depending on the result
- Stop TouchEngine: destroys the TE Instance
- ReloadTox: does nothing at runtime, reloads stored parameters for UI use

### TOPs

There is no TOPs specific methods at the moment. TouchEngine TOPs are treated as UE Texture2D values.

### CHOPs

- Get Channel: returns a float array of the channel at channel index
- Get Channel by Name: same as get channel, but gets the value by name instead of by index
- Clear: empties the CHOP struct

### DATs

- Get Cell: returns FString of value at index row, col
- Get Row: returns a FString array of the row at index row
- Get Column: returns a FString array of the column at index col
- Get Cell by Name: same as get cell, but uses the first cell of the row and column as their names
- Get Row by Name: same as get row, but uses the first cell of the row as its name
- Get Column by Name: same as get column, but uses the first cell of the column as its name