# CheckBox Widget

This example demonstrates the `CheckBox` widget, showcasing two checkboxes in a row, one checked ("On") and one unchecked ("Off").

## CheckBox with Text Labels

### Code

```c++
rcnew HLayout{
    rcnew CheckBox{ rcnew Text{ "On" }, value = true },
    rcnew CheckBox{ rcnew Text{ "Off" }, value = false } 
}
```

### Result

<img src="../../images/widget-checkbox.png" width="180" alt="CheckBox with text labels">


## CheckBox animation


<img src="../../images/checkbox.webp" width="270" alt="CheckBox animation">
