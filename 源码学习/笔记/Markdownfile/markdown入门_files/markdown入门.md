# <font face="仿宋">基础用法</font>

## 1.标题
#一级标题
##二级标题
> 支持到六级标题

## 2.引用
>这是一个引用

## 3.列表 
1. 无序列表
   - 列表 1
   - 列表 2
   - 列表 3
2. 有序列表
   1. 列表1
   2. 列表2
   3. 列表3
3. todolist
   - [x] a
   - [ ] b
   - [ ] c
4. 表格
    | 左对齐 | 居中 | 右对齐 |
    | :----- | :-----: | -----: |
    | a | b | c |

## 5.段落
- 分割线 :--- 
  >三个以上的"-"或"*"
  ---
  ***
- 字体
  | 字体 | 语法 |
  | :---: | :---: |
  | *斜体* | ```*-*``` |
  | ==高亮== | ```==_==``` |
  | **粗体** | ```**_**``` |
  | ***斜粗体*** | ```***_***``` |
  | ~~删除~~ |```~~_~~``` |
  | <u>下划线</u> | ```<u></u>``` |
- 脚注
  markdown[^1]
  [^1]: markdown好用


## 6.代码
>多句代码
```
//在两行" ``` "之间给出你的代码

#include<iostream>

using namspace std;

const int N = 1e5 + 10;

int main()
{
    cout << hello world << endl;
    return 0;
}


```
>单句代码
`printf("%d", N);`

## 7.超链接
- [markdown语法教程]：https://www.bilibili.com/video/BV1bK4y1i7BY/?spm_id_from=333.337.search-card.all.click&vd_source=3b084c1c9fffb7e9de2583543ab77256
- 更多教程请[点击链接][教程]

[教程]:https://daringfireball.net/projects/markdown/

## 8.图片
> 这里使用了paste image将图片拖拽到代码中，图片保存在本地

![alt text](<屏幕截图 2024-07-13 141630.png>)