struct AddPayload {
    int a;
    int b;
    int result;
};

extern "C" bool AddWorker(void* data, int length) {
    if (!data || length < static_cast<int>(sizeof(AddPayload))) {
        return false;
    }
    auto* payload = static_cast<AddPayload*>(data);
    payload->result = payload->a + payload->b;
    return true;
}