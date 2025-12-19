#!/bin/sh
# run_tests.sh

echo "== Тест: server + client1 + client2 =="

# Чистим старые выводы
rm -f server_out.txt client1_out.txt client2_out.txt

echo ">> Стартуем сервер в фоне"

(
    ( sleep 3; printf '\n' ) | ./server > server_out.txt 2>&1
) &
SERVER_WRAPPER_PID=$!

# Даём серверу время создать РОП и семафоры
sleep 1

echo ">> Запуск client1"
./client1 > client1_out.txt 2>&1 || {
    echo "client1 завершился с ошибкой"
    cat client1_out.txt || true
    kill "$SERVER_WRAPPER_PID" || true
    exit 1
}

echo ">> Запуск client2"
./client2 > client2_out.txt 2>&1 || {
    echo "client2 завершился с ошибкой"
    cat client2_out.txt || true
    kill "$SERVER_WRAPPER_PID" || true
    exit 1
}

echo ">> Ждём завершения сервера"
wait "$SERVER_WRAPPER_PID"

echo "--- Вывод сервера ---"
cat server_out.txt

echo "--- Вывод client1 ---"
cat client1_out.txt

echo "--- Вывод client2 ---"
cat client2_out.txt

# Примитивные проверки, что всё вообще отработало

grep -q "Клиент 1" client1_out.txt

grep -q "Клиент 2" client2_out.txt

grep -q "Сообщения от клиентов" server_out.txt

grep -q "Client1:" server_out.txt
grep -q "Client2:" server_out.txt

echo "== Все тесты пройдены =="
