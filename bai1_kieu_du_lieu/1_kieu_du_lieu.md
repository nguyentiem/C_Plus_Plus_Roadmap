# Cơ bản 

## Kiểu dữ liệu  

Tương tự c nó có các kiểu dữ liệu nguyên thủy cơ bản kiểu biết trước kích thước và kiểu không biết trước kích thước 

trong c++ bool là kiểu dữ liệu của ngôn ngữ còn c thì phải thông qua thư viện bản chất là short , c++ dùng được trong template 

wchar_t, char16_t, char32_t trên c++ là kiểu dữ liệu gốc còn trên c là typdef 

### con trỏ 

khi ép kiểu con trỏ void C tự động ép kiểu nhưng trong c++ cần ghi rõ 

khác biệt giữa malloc/freee và new/delete
    new trả về đúng kiểu dữ liệu  cấp phát 
    nó sẽ gọi construtor cà deconstructor 
2. tham chiếu 

trong c++ có thêm kiểu dữ liệu tham chiếu và bắt buộc phải gán khi bắt đầu 

### Struct 

struct trong C++ có thể có hàm 
giống class nhưng default access là public 


### namesapce

tương tự package trong java nó giúp tổ chức phạm vi các hàm , biến và object có cùng tên
tại sao có các trường hợp này : 
    trong nhiều lĩnh vực có các thuật ngữ đối tượng cùng tên nhưng chúng vốn chả có liên quan gì đến nhau thì khi có một tầng gọi ra thì cần namespace để viết sử dụng cái nào 

hoàn toàn có thể cùng một namespace trong nhiều file 

### overloading và default param

cho phép các hàm trùng tên chỉ cần khác chữ kí 



### các loại ép kiểu 
```
Cú pháp	Kiểm tra chính	Mục đích
static_cast<T>(x)	Lúc biên dịch	Chuyển đổi kiểu thông thường
dynamic_cast<T>(x)	Lúc chạy	Kiểm tra kiểu thật trong kế thừa
const_cast<T>(x)	Lúc biên dịch	Thêm hoặc bỏ const
reinterpret_cast<T>(x)	Rất ít kiểm tra	Diễn giải địa chỉ/bit theo kiểu khác
```