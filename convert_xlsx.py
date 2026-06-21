import csv
import zipfile
import xml.etree.ElementTree as ET

# 读取xlsx文件（实际上是zip格式）
xlsx_file = 'C:\\Users\\Administrator\\Desktop\\STM32H7_LVGL\\PickAndPlace_PCB1_2026_06_08.xlsx'

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
                        # 检查是否是共享字符串引用
                        t = cell.get('t')
                        if t == 's':
                            # 共享字符串索引
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
            
            # 打印数据
            for row in rows_data[:50]:  # 只打印前50行
                print('\t'.join(str(cell) for cell in row))
                
except Exception as e:
    print(f"Error: {e}")
