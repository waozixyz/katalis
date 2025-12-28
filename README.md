# Katalis

A Kryon framework application demonstrating both traditional C API and the new DSL syntax.

## Project Structure

```
katalis/
├── LICENSE              # 0BSD license
├── kryon.toml          # Kryon configuration
├── shell.nix           # Nix development environment
├── README.md           # This file
└── src/
    ├── main.c          # Traditional Kryon C API example
    └── main_dsl.c      # New DSL syntax example
```

## Kryon C DSL

This project showcases the new **C macro-based DSL** for Kryon, providing beautiful declarative syntax similar to Nim DSL and .kry files.

### Syntax Comparison

**Traditional C API (main.c) - 30 lines:**
```c
#include <kryon.h>

int main(void) {
    kryon_init("Katalis", 800, 600);

    IRComponent* root = kryon_get_root();

    IRComponent* column = kryon_column();
    kryon_set_width(column, 100.0, "%");
    kryon_set_height(column, 100.0, "%");
    kryon_set_background(column, 0x101820);
    kryon_set_justify_content(column, IR_ALIGNMENT_CENTER);
    kryon_set_align_items(column, IR_ALIGNMENT_CENTER);

    IRComponent* container = kryon_container();
    kryon_set_width(container, 200.0, "px");
    kryon_set_height(container, 100.0, "px");
    kryon_set_background(container, 0x191970);
    kryon_set_justify_content(container, IR_ALIGNMENT_CENTER);
    kryon_set_align_items(container, IR_ALIGNMENT_CENTER);

    IRComponent* text = kryon_text("Hello World");
    kryon_set_color(text, KRYON_COLOR_YELLOW);
    kryon_set_font_size(text, 24.0);

    kryon_add_child(container, text);
    kryon_add_child(column, container);
    kryon_add_child(root, column);

    kryon_finalize("output.kir");
    kryon_cleanup();
    return 0;
}
```

**New DSL Syntax (main_dsl.c) - 17 lines (43% shorter!):**
```c
#include <kryon_dsl.h>

int main(void) {
    kryon_init("Katalis", 800, 600);

    KRYON_APP(
        COLUMN(
            FULL_SIZE,
            BG_COLOR(0x101820),
            FLEX_CENTER,

            CONTAINER(
                WIDTH(200), HEIGHT(100),
                BG_COLOR(0x191970),
                FLEX_CENTER,

                TEXT("Hello World", COLOR_YELLOW, FONT_SIZE(24))
            )
        )
    );

    kryon_finalize("output.kir");
    kryon_cleanup();
    return 0;
}
```

### DSL Benefits

1. **43% less code** - More concise and readable
2. **Declarative syntax** - Visual nesting matches UI structure
3. **No manual parent-child** - Automatic tree building
4. **Property shortcuts** - `FLEX_CENTER`, `FULL_SIZE`, named colors
5. **Zero runtime overhead** - Macros expand to existing API calls
6. **Backwards compatible** - Can mix DSL and traditional API

## Available DSL Macros

### Layout Components
- `COLUMN(...)` - Vertical flex container
- `ROW(...)` - Horizontal flex container
- `CONTAINER(...)` - Generic container
- `CENTER(...)` - Center alignment

### UI Components
- `TEXT(content, ...)` - Text display
- `BUTTON(label, ...)` - Button
- `INPUT(placeholder, ...)` - Text input
- `CHECKBOX(label, checked, ...)` - Checkbox
- `DROPDOWN(placeholder, ...)` - Dropdown
- `IMAGE(src, alt, ...)` - Image

### Table Components
- `TABLE(...)`, `TABLE_HEAD(...)`, `TABLE_BODY(...)`
- `TR(...)` - Table row
- `TH(content, ...)` - Table header
- `TD(content, ...)` - Table cell

### Dimension Properties
- `WIDTH(val)`, `HEIGHT(val)` - Dimensions in pixels
- `WIDTH_PCT(val)`, `HEIGHT_PCT(val)` - Dimensions in percentage
- `FULL_SIZE` - Shortcut for 100% width and height
- `SIZE(val)`, `SQUARE(val)` - Set both dimensions

### Color Properties
- `BG_COLOR(hex)` - Background color (hex value)
- `TEXT_COLOR(hex)` - Text color
- `COLOR_RED`, `COLOR_GREEN`, `COLOR_BLUE`, etc. - Named colors
- `BG_RED`, `BG_GREEN`, `BG_BLUE`, etc. - Named background colors

### Layout Properties
- `JUSTIFY_CENTER`, `JUSTIFY_START`, `JUSTIFY_END`, `JUSTIFY_BETWEEN`
- `ALIGN_CENTER`, `ALIGN_START`, `ALIGN_END`, `ALIGN_STRETCH`
- `FLEX_CENTER` - Shortcut for center both axes
- `GAP(val)` - Gap between flex items
- `FLEX_GROW(val)`, `FLEX_SHRINK(val)`, `FLEX_WRAP`

### Spacing Properties
- `PADDING(val)`, `MARGIN(val)` - Uniform spacing
- `PADDING_TRBL(t,r,b,l)`, `MARGIN_TRBL(t,r,b,l)` - Individual sides
- `PADDING_H(val)`, `PADDING_V(val)` - Horizontal/vertical shortcuts
- `MARGIN_H(val)`, `MARGIN_V(val)` - Horizontal/vertical shortcuts

### Typography Properties
- `FONT_SIZE(size)` - Font size
- `FONT_FAMILY(name)` - Font family
- `FONT_BOLD`, `FONT_ITALIC` - Font styles
- `FONT_THIN`, `FONT_NORMAL`, `FONT_SEMI_BOLD`, etc. - Font weights
- `TEXT_ALIGN_LEFT`, `TEXT_ALIGN_CENTER`, `TEXT_ALIGN_RIGHT`
- `LINE_HEIGHT(val)`, `LETTER_SPACING(val)`

### Border & Effects
- `BORDER_WIDTH(val)`, `BORDER_RADIUS(val)`, `BORDER_COLOR(hex)`
- `OPACITY(val)` - Opacity (0.0 - 1.0)
- `Z_INDEX(val)` - Z-index for layering
- `VISIBLE(bool)`, `HIDDEN` - Visibility

### Event Handlers
- `ON_CLICK(handler)` - Click event
- `ON_CHANGE(handler)` - Change event
- `ON_HOVER(handler)` - Hover event
- `ON_FOCUS(handler)` - Focus event

## Building

```bash
# Enter nix development shell
nix-shell

# Build with kryon CLI
kryon build
```

## License

0BSD - See LICENSE file
