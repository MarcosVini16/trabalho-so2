class SmartData {
public:
    SmartData() = default;
    ~SmartData() = default;

    void setData(int data) {
        this->data = data;
    }

    int getData() const {
        return data;
    }

    private:
        int data{0};
};