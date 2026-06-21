import zipfile
import xml.etree.ElementTree as ET

xlsx_file = 'C:\\Users\\Administrator\\Desktop\\STM32H7_LVGL\\PickAndPlace_PCB1_2026_06_08.xlsx'

components = {}

try:
    with zipfile.ZipFile(xlsx_file, 'r') as zip_ref:
        # 读取共享字符串
        shared_strings = []
        try:
            with zip_ref.open('xl/sharedStrings.xml') as f:
                tree = ET.parse(f)
                root = tree.getroot()
                ns = {'ns': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
                for si in root.findall('.//ns:si', ns):
                    t = si.find('.//ns:t', ns)
                    if t is not None:
                        shared_strings.append(t.text)
        except:
            pass
        
        # 读取第一个工作表
        with zip_ref.open('xl/worksheets/sheet1.xml') as f:
            tree = ET.parse(f)
            root = tree.getroot()
            ns = {'ns': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
            
            rows_data = []
            for row in root.findall('.//ns:row', ns):
                row_data = []
                for cell in row.findall('.//ns:c', ns):
                    v = cell.find('ns:v', ns)
                    if v is not None:
                        t = cell.get('t')
                        if t == 's':
                            idx = int(v.text)
                            if idx < len(shared_strings):
                                row_data.append(shared_strings[idx])
                            else:
                                row_data.append(v.text)
                        else:
                            row_data.append(v.text)
                    else:
                        row_data.append('')
                rows_data.append(row_data)
            
            # 解析数据（跳过表头）
            for row in rows_data[1:]:
                if len(row) >= 6:
                    designator = row[0]
                    device = row[1] if len(row) > 1 else ''
                    mid_x = row[4] if len(row) > 4 else ''
                    mid_y = row[5] if len(row) > 5 else ''
                    
                    if designator and mid_x and mid_y:
                        try:
                            # 转换坐标（去除mm单位）
                            x = float(mid_x.replace('mm', ''))
                            y = float(mid_y.replace('mm', ''))
                            components[designator] = {
                                'device': device,
                                'x': x,
                                'y': y
                            }
                        except:
                            pass

    # 分析关键器件位置
    print("=" * 80)
    print("关键器件坐标分析")
    print("=" * 80)
    
    key_components = {
        'U1': 'STM32H7 MCU',
        'U2': 'SDRAM',
        'SPI2': 'SPI Flash',
        'U9': 'ESP32-S3',
        'U11': 'MP1584 电源',
        'J1': 'LCD FPC接口',
        'J2': 'Camera FFC接口',
        'USB1': 'USB-C接口',
        'SD1': 'SD卡座',
        'U3': 'CH343P USB转串口',
        'U7': 'MPU6050',
        'U13': 'MAX98357音频功放',
        'Y1': '32.768K晶振',
        'XY1': '主晶振',
        'L1': 'MP1584电感'
    }
    
    for ref, name in key_components.items():
        if ref in components:
            comp = components[ref]
            print(f"{ref:8s} ({name:20s}): X={comp['x']:7.2f}mm, Y={comp['y']:7.2f}mm")
    
    # 计算关键距离
    print("\n" + "=" * 80)
    print("关键距离分析")
    print("=" * 80)
    
    if 'U1' in components and 'U2' in components:
        dx = components['U2']['x'] - components['U1']['x']
        dy = components['U2']['y'] - components['U1']['y']
        dist = (dx**2 + dy**2)**0.5
        print(f"STM32 ↔ SDRAM距离: {dist:.2f}mm {'✅' if dist < 20 else '⚠️ 建议<20mm'}")
    
    if 'U1' in components and 'SPI2' in components:
        dx = components['SPI2']['x'] - components['U1']['x']
        dy = components['SPI2']['y'] - components['U1']['y']
        dist = (dx**2 + dy**2)**0.5
        print(f"STM32 ↔ SPI Flash距离: {dist:.2f}mm {'✅' if dist < 25 else '⚠️ 建议<25mm'}")
    
    if 'U1' in components and 'U11' in components:
        dx = components['U11']['x'] - components['U1']['x']
        dy = components['U11']['y'] - components['U1']['y']
        dist = (dx**2 + dy**2)**0.5
        print(f"STM32 ↔ MP1584距离: {dist:.2f}mm {'✅ 足够远' if dist > 15 else '⚠️ 建议>15mm避免干扰'}")
    
    if 'U1' in components and 'Y1' in components:
        dx = components['Y1']['x'] - components['U1']['x']
        dy = components['Y1']['y'] - components['U1']['y']
        dist = (dx**2 + dy**2)**0.5
        print(f"STM32 ↔ RTC晶振距离: {dist:.2f}mm {'✅' if dist < 15 else '⚠️ 建议<15mm'}")
    
    if 'U1' in components and 'XY1' in components:
        dx = components['XY1']['x'] - components['U1']['x']
        dy = components['XY1']['y'] - components['U1']['y']
        dist = (dx**2 + dy**2)**0.5
        print(f"STM32 ↔ 主晶振距离: {dist:.2f}mm {'✅' if dist < 15 else '⚠️ 建议<15mm'}")
    
    # PCB尺寸估算
    print("\n" + "=" * 80)
    print("PCB尺寸估算")
    print("=" * 80)
    
    all_x = [c['x'] for c in components.values()]
    all_y = [c['y'] for c in components.values()]
    
    min_x, max_x = min(all_x), max(all_x)
    min_y, max_y = min(all_y), max(all_y)
    width = max_x - min_x
    height = max_y - min_y
    
    print(f"X范围: {min_x:.2f}mm ~ {max_x:.2f}mm")
    print(f"Y范围: {min_y:.2f}mm ~ {max_y:.2f}mm")
    print(f"估算尺寸: {width:.1f}mm × {height:.1f}mm")
    
    # 功能分区分析
    print("\n" + "=" * 80)
    print("功能分区建议")
    print("=" * 80)
    
    if 'U1' in components:
        u1_x, u1_y = components['U1']['x'], components['U1']['y']
        print(f"\nSTM32核心区中心: ({u1_x:.1f}, {u1_y:.1f})")
        
        # 找出靠近STM32的器件
        near_mcu = []
        for ref, comp in components.items():
            if ref != 'U1':
                dx = comp['x'] - u1_x
                dy = comp['y'] - u1_y
                dist = (dx**2 + dy**2)**0.5
                if dist < 30:  # 30mm范围内
                    near_mcu.append((ref, dist))
        
        near_mcu.sort(key=lambda x: x[1])
        print("\nSTM32周围30mm内器件（按距离排序）:")
        for ref, dist in near_mcu[:10]:
            device = components[ref].get('device', '')
            print(f"  {ref:8s} ({device:15s}): {dist:6.2f}mm")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
