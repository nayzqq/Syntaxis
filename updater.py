import os
import re
import urllib.request

# Прямые (Raw) ссылки на файлы a2x/cs2-dumper
URLS = [
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.cs",
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.cs",
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/buttons.cs"
]

def main():
    target_file = "entity.hpp"

    if not os.path.exists(target_file):
        print(f"[-] Ошибка: Файл '{target_file}' не найден в текущей папке!")
        return

    # 1. Читаем текущий entity.hpp
    with open(target_file, 'r', encoding='utf-8') as f:
        cpp_code = f.read()

    # 2. Ищем все оффсеты в entity.hpp
    existing_offsets = re.findall(r'(?:const|constexpr)\s+(?:std::)?(?:uintptr_t|ptrdiff_t|uint32_t|uint64_t)\s+([a-zA-Z0-9_]+)\s*=', cpp_code)

    if not existing_offsets:
        print(f"[-] Оффсеты в {target_file} не найдены. Убедитесь, что они имеют тип const/constexpr uintptr_t.")
        return

    print(f"[*] Найдено {len(existing_offsets)} оффсетов в {target_file}. Начинаем проверку...\n")

    # 3. Скачиваем актуальные дампы с GitHub
    print("[*] Скачиваем свежие дампы с a2x/cs2-dumper...")
    fetched_offsets = {}

    for url in URLS:
        try:
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req) as response:
                text = response.read().decode('utf-8')
                matches = re.findall(r'([a-zA-Z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+|\d+);', text)
                for name, value in matches:
                    fetched_offsets[name] = value
            print(f"[+] Скачан: {url.split('/')[-1]} ({len(matches)} записей)")
        except Exception as e:
            print(f"[-] Ошибка при скачивании {url.split('/')[-1]}: {e}")

    # 4. Обновляем entity.hpp
    print()
    changes_made = 0

    for name in existing_offsets:
        if name in fetched_offsets:
            new_value = fetched_offsets[name]

            pattern = r"((?:const|constexpr)\s+(?:std::)?(?:uintptr_t|ptrdiff_t|uint32_t|uint64_t)\s+" + re.escape(name) + r"\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)"

            def replacer(match, new_value=new_value, name=name):
                old_value = match.group(2)
                if old_value.lower() != new_value.lower():
                    print(f"[+] Обновлен {name}: {old_value} -> {new_value}")
                return match.group(1) + new_value + match.group(3)

            new_cpp_code, count = re.subn(pattern, replacer, cpp_code)

            if count > 0 and new_cpp_code != cpp_code:
                cpp_code = new_cpp_code
                changes_made += 1

    # 5. Сохраняем результат
    if changes_made > 0:
        with open(target_file, 'w', encoding='utf-8') as f:
            f.write(cpp_code)
        print(f"\n[+] Успешно! Файл {target_file} перезаписан (обновлено {changes_made} значений).")
    else:
        print(f"[*] Обновление не требуется. Все оффсеты в {target_file} уже актуальны.")

if __name__ == "__main__":
    main()
    input("\nНажми Enter для выхода...")
