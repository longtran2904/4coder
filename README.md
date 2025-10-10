# 4coder_long
My personal [4coder](https://github.com/4coder-archive/4coder) custom layer (_WIP_)

 <img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/462a03f2-4891-41f1-9e95-1d9621841612" />

This repository includes the built executable, so if you’re not interested in the source code, simply download the entire repo and run `4ed.exe`.

If you only want my customization layer, copy the `4coder_long` folder into your `custom` folder.\
Then, you can either compile `4coder_long.cpp` into `custom_4coder.dll` yourself, or simply run `build.bat`.

The code currently relies heavily on `4coder_fleury_lang` and `4coder_fleury_index`.\
Removing this is on my roadmap — I’ve already modified most of these files anyway.

## Features
For a detailed list of features, see `4coder_long.cpp`.\
Below are some of the most notable ones:

### Custom Render Hook
By registering a custom rendering hook, we can begin customizing the rendering process.\
(See `Long_Render`, `Long_Render_Buffer`, and `Long_WholeScreenRender` for more details).\
For example, here’s my custom file bar design:

![4coder_file_bar](https://github.com/user-attachments/assets/716ee90d-2c33-409d-b36d-24b87cbc0fa6)

### Error Annotations
Highlights lines containing errors and displays the corresponding message at the end of the line:

<img width="1280" height="320" alt="image" src="https://github.com/user-attachments/assets/f668b1b4-93c2-4799-8f4e-5b45e39b5971" />

Also works with `.4coder` files:

<img width="1280" height="320" alt="image" src="https://github.com/user-attachments/assets/d4e913a6-f34c-45e3-8df5-636e779b8ae4" />

### Digit Rendering
Alternate the color of every 3 decimal digits or 4 binary/hex digits for visual grouping, and highlight 8-digit numbers with a color based on their ARGB value.

<img width="840" height="120" alt="image" src="https://github.com/user-attachments/assets/eaed5c0d-ce6b-4b35-b0d8-08827dc7c5fa" />

### Theme Lister
Smoothly switch between themes with `long_theme_lister`.

![4coder_theme](https://github.com/user-attachments/assets/df1bd47b-863c-4b59-8353-b85625784c5a)

The editor also automatically hot-reloads when a theme file is saved and refreshes its associated color table.\
This is where the earlier hex coloring feature really shines:

![4coder_theme_2](https://github.com/user-attachments/assets/f29a517d-22d6-4b12-92a6-4cc962e4bb6e)

### Whitespace Highlight

![4coder_whitespace](https://github.com/user-attachments/assets/3aff3056-5afb-4c7e-8440-c3339db7798c)

### Comment Styles
Here’s a preview of how different comment styles look:

<img width="960" height="240" alt="image" src="https://github.com/user-attachments/assets/6aa45fab-6fdb-49d4-b784-e9755b9cd598" />

### Token Occurrences
Highlights all occurrences of the identifier under the cursor.\
You can also easily move between them using `long_move_up_token_occurrence` and `long_move_down_token_occurrence`.

![4coder_token_occurence](https://github.com/user-attachments/assets/dfa2cd22-f143-4bf5-99ce-99b306410f81)

### Point Stack
By tracking the cursor position before and after any jump-related command, you can jump back and forward using `long_undo_jump` and `long_redo_jump`.\
Visualize the entire jump stack with `long_point_lister`.

![4coder_jump_stack](https://github.com/user-attachments/assets/7ff105e3-471e-41d7-8024-9ae6ae430b88)

### Multi-Cursor
BYP's amazing [4coder_multi_cursor.cpp](https://github.com/B-Y-P/4coder_qol/blob/main/plugins/4coder_multi_cursor.cpp) plugin.

![4coder_multi_cursor](https://github.com/user-attachments/assets/d24d9726-ebd0-4552-82dd-739e9fdd0c01)

### `long_replace_in_range`

![4coder_replace_range](https://github.com/user-attachments/assets/ecd409f9-9021-429f-a09d-d8f72730cfee)

### `long_list_all_locations_xxx`

![4coder_list_locations](https://github.com/user-attachments/assets/ef27d1d3-51f9-4af4-846f-3f9ebba71b6a)

### Code Index
Each loaded code file is parsed and indexed into a symbol table.\
This enables advanced features like symbol lookup, code peeking, position-based tooltips, go-to definition, and definition search.

https://github.com/user-attachments/assets/ee9ba110-d419-42f3-948b-466a40ac520b

### Lister
The new lister system includes several improvements over the default one:
- better filtering (supports inclusive, exclusive, and tag-based searches)
- cleaner UI design (displays the total count and current index)
- only renders visible items
- improved input handling (copy–paste, clear all, etc)
- each item can include a header, a tooltip, and an optional preview (e.g, code peek for the index lister, docs for the command lister)

https://github.com/user-attachments/assets/2efa2a86-92ba-4e09-bd32-a0adfcd1c24a
