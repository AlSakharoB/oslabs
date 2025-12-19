#!/bin/sh
# run_tests.sh

rm -rf out
mkdir out

echo "== Тест: server + client1 + client2 =="

# Чистим старые выводы и сокеты
rm -f out/* unix_sock_18


echo ">> Создаём small.txt (<100 bytes)"
echo "hello world" > small.txt

echo ">> Создаём big.txt (>100 bytes)"
# Запишем текст больше 100 байт
printf 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n' > big.txt

touch small.txt big.txt

echo "Размер small.txt:"
wc -c small.txt
echo "Размер big.txt:"
wc -c big.txt


echo ">> Стартуем сервер в фоне"
./server > out/server_out.txt 2>&1 &
SERVER_PID=$!

# Даём время серверу создать сокет
sleep 1


echo ">> Запуск client1"
./client1 > out/client1_out.txt 2>&1 || {
    echo "client1 завершился с ошибкой"
    cat out/client1_out.txt || true
    kill "$SERVER_PID" || true
    exit 1
}

echo ">> Запуск client2"
./client2 > out/client2_out.txt 2>&1 || {
    echo "client2 завершился с ошибкой"
    cat out/client2_out.txt || true
    kill "$SERVER_PID" || true
    exit 1
}

echo ">> Ждём завершения сервера"
wait "$SERVER_PID"


echo "--- Вывод сервера ---"
cat out/server_out.txt

echo "--- Вывод client1 ---"
cat out/client1_out.txt

echo "--- Вывод client2 ---"
cat out/client2_out.txt


grep -q "Клиент 1: подготовлено" out/client1_out.txt
grep -q "Клиент 1: отправлено." out/client1_out.txt

grep -q "Клиент 2: подготовлено" out/client2_out.txt
grep -q "Клиент 2: отправлено." out/client2_out.txt

grep -q "Сервер: получил список от клиента 1" out/server_out.txt
grep -q "Сервер: получил список от клиента 2" out/server_out.txt

grep -q "Файлы, присутствующие в обоих списках" out/server_out.txt

# сервер завершился
grep -q "Сервер: завершение." out/server_out.txt

grep -q "small.txt" out/server_out.txt || {
    echo "ОШИБКА: small.txt должен быть в пересечении!"
    exit 1
}

if grep -q "big.txt" out/server_out.txt; then
    echo "ОШИБКА: big.txt не должен попасть в пересечение."
    exit 1
fi

echo "== Все тесты пройдены =="
