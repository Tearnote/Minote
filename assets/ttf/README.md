# Font generation
the `.json` and `.png` files were created with
[`msdf-bmfont-xml`](https://github.com/soimy/msdf-bmfont-xml).

Arguments used:
```
msdf-bmfont -f json -s 64 -i charmap.txt -m 1024,1024 -r 4 -p 1 -b 3 --pot --square font.ttf
```
