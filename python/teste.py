from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient('127.0.0.1', port=502, timeout=3)
if not client.connect():
    print("❌ Falha ao conectar")
    exit(1)

# Ajuste aqui para o número de coils que seu slave suporta (máx = 4)
REQUEST_COUNT = 4

rr = client.read_discrete_inputs(address=0, count=REQUEST_COUNT, slave=1)
if rr.isError():
    print("❌ Erro Modbus:", rr)
else:
    # rr.bits sempre tem len % 8 == 0, então fatie até REQUEST_COUNT
    coils = rr.bits[:REQUEST_COUNT]
    print(f"✅ {REQUEST_COUNT} coils lidos:", coils)

client.close()