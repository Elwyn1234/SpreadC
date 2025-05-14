# User Input

## Input Handling - Technical Considerations

- handle different inputs in a selection of technically different ways
    - non-combination commands (take count as an argument). Handle this simply by switching over the given input/command and committing/handling that input immediately (each input has a unique handler implementation). This is what I was doing at first and that is the solution i should use for these basic commands. Things like `count` can be given to the command as an argument. This covers most inputs and includes
        - insert mode commands: i o a
        - undo redo commands: u r U
        - some operator commands: x D
        - motion commands when not provided with an operator: G gg M h j k l
    - [count]<action><motion> commands and {[Block-/Row-]Visual}<action> commands. It is likely to be effective to handle both these types of command using this solution. First calculate a list of cells or a region of cells to act upon. Then handle the command by switching over the given operator and committing/handling that input (each operator has a unique handler implementation). Give the cell region/list as an argument to the handler implementation.
        - It is likely that the above non-combination handler can also be used as the handler for these combination commands. For example, non-combination handler for 'x' would be called once for each cell to be deleted, as calculated by the handler for the combination command.
    - Both the above handlers should be backed by granular/modular spreadsheet "api". This api can be called not only by the input handling but also by plugin scripts. In the case of input handling, the handlers may call the API a number of times to implement the desired response to the user input.
        - perhaps bad example but, 'k' input would call api function CURSOR_ROW_UP and conditionally call SCROLL_ROW_UP if the cursor would otherwise go off-screen. The <CTRL>-y input will also call SCROLL_ROW_UP (this time unconditionally) and it will conditionally call CURSOR_ROW_UP if the cursor would otherwise go off-screen.
        - This will be helpful in simplifying my implementation because these modular functions are context independent (which seems to be the most confusing part at the moment) and the requirements of these modular functions are therefore well-defined and easy to understand.


## Supported Text Input 
- Supported
  - Latin-1
  - the renderer support UTF-8 although the input manager does not (only supports Latin-1)
- Unsupported
  - Is Euro sign support (i.e. shift-alt-4 or whatever it is)

<!--NOTES - ACTIONS:-->
<!--- an interaction with the spreadsheet is a composition of [count]<action>{motion}-->
<!--  - In cases where the action can either act on rows or columns, the user can specify this using the <count> component.-->
<!--    - a <count> of 18r would act on 19 rows (the current row plus 18 rows above or below)-->
<!--    - 18c would similarly act on the columns-->
<!--    - full example 18rdj - i.e. delete 19 rows from the selected row, downwards-->
<!--      - 18r is the count-->
<!--      - d is the action-->
<!--      - j is the motion-->
<!---->
