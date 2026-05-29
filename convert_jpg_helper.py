import os
from PIL import Image

folder = r'E:\PHOTO'

if not os.path.isdir(folder):
    print('[ERROR] Folder not found: ' + folder)
    exit(1)

exts = ('.jpg', '.jpeg')
files = [f for f in os.listdir(folder) if f.lower().endswith(exts)]

if not files:
    print('[INFO] No JPG files found in ' + folder)
else:
    ok = 0
    fail = 0
    for f in files:
        path = os.path.join(folder, f)
        try:
            img = Image.open(path).convert('RGB')
            img.save(path, 'JPEG', progressive=False, quality=85, subsampling=0)
            print('[OK]   ' + f)
            ok += 1
        except Exception as e:
            print('[FAIL] ' + f + ': ' + str(e))
            fail += 1
    print()
    print('Done: ' + str(ok) + ' OK, ' + str(fail) + ' failed')
