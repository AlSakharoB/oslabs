#!/bin/sh
# run_tests.sh

echo "== Test 1: один обычный файл =="

rm -rf testdir_1
mkdir testdir_1

(
    cd testdir_1 || exit 1

    printf "one\ntwo\nthree\n" > file1.txt

    ../client
    ../server > out1.txt

    echo "--- Вывод сервера ---"
    cat out1.txt

    grep -q "file: file1.txt" out1.txt
    grep -q "lines: 3" out1.txt
    grep -q "messages_in_queue: 1" out1.txt
)

echo "== Тест 1 пройден =="


echo "== Test 2: нет обычных файлов =="

rm -rf testdir_2
mkdir testdir_2

(
    cd testdir_2 || exit 1

    mkdir subdir

    ../client 2> subdir/client_err.txt
    ../server > out2.txt

    echo "--- Вывод client ---"
    cat subdir/client_err.txt || true

    echo "--- Вывод сервер ---"
    cat out2.txt

    grep -q "Нет обычных файлов в каталоге" subdir/client_err.txt

    grep -q "file: (none)" out2.txt
    grep -q "lines: error_opening_file" out2.txt
    grep -q "messages_in_queue: 1" out2.txt
)

echo "== Тест 2 пройден =="

echo "== Все тесты пройдены =="
