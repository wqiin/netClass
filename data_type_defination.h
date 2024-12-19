#ifndef DATA_TYPE_DEFINATION_H
#define DATA_TYPE_DEFINATION_H

#include <iostream>
#include <string>
#include <cstdint>

struct StDate{
    int nYear = -1;
    int nMonth = -1;
    int nDay  = -1;

    friend std::istream & operator>>(std::istream & is, StDate & date) {
        char chColon1 = 0, chColon2 = 0;
        is >> date.nYear >> chColon1 >> date.nMonth >> chColon2 >> date.nDay;

        //check whether parsing ok
        if (!is || chColon1 != '-' || chColon2 != '-') {
            is.setstate(std::ios::failbit); //setting failbit flag, marking parsing failure
        }

        return is;
    }

};

struct StTime{
    int nHour = -1;
    int nMinute = -1;
    int nSecond = -1;
    /*
    std::istream & operator>>(std::istream & is) {
        char chColon1 = 0, chColon2 = 0;
        is >> nHour >> chColon1 >> nMinute >> chColon2 >> nSecond;

        //check whether parsing ok
        if (!is || chColon1 != ':' || chColon2 != ':') {
            is.setstate(std::ios::failbit);  //setting failbit flag, marking parsing failure
        }

        return is;
    }
*/

    //overload as a friend function
    friend std::istream & operator>>(std::istream & is, StTime & time) {
        char chColon1 = 0, chColon2 = 0;
        is >> time.nHour >> chColon1 >> time.nMinute >> chColon2 >> time.nSecond;

        //check whether parsing ok
        if (!is || chColon1 != ':' || chColon2 != ':') {
            is.setstate(std::ios::failbit); //setting failbit flag, marking parsing failure
        }

        return is;
    }
};

struct StDateTime{
    StDate date;
    StTime time;

    //overload as a friend function
    friend std::istream & operator >>(std::istream & is, StDateTime & dateTime) {
        is >> dateTime.date >> dateTime.time;

        //check whether parsing ok
        if (!is) {
            is.setstate(std::ios::failbit); //setting failbit flag, marking parsing failure
        }

        return is;
    }
};

template<class T>
struct StPoint{
    T x = 0;
    T y = 0;

    //overload as a friend function
    friend std::istream & operator >>(std::istream & is, StPoint & point){
        char chLeftBracket = 0, chColon = 0, chRightBracket = 0;
        is >> chLeftBracket >> point.x >> chColon >> point.y >> chRightBracket;

        if (!is || chLeftBracket != '(' || chColon != ',' || chRightBracket != ')') {
            is.setstate(std::ios::failbit); //setting failbit flag, marking parsing failure
        }

        return is;
    }
};

template<class T>
struct StRect{
    StPoint<T> leftTop;
    StPoint<T> rightBottom;

    //overload as a friend function
    friend std::istream & operator >>(std::istream & is, StRect & rect){
        char chLeftBracket = 0, chColon = 0, chRightBracket = 0;
        is >> chLeftBracket >> rect.leftTop >> chColon >> rect.rightBottom >> chRightBracket;

        if (!is || chLeftBracket != '[' || chColon != ',' || chRightBracket != ']') {
            is.setstate(std::ios::failbit); //setting failbit flag, marking parsing failure
        }

        return is;
    }
};

template<class T>
struct StCircle{
    StPoint<T> pivol;
    T radius = 0;
};




typedef struct ST_course{
    std::int64_t nID = 0;
    std::string strCourseName;
    std::int64_t nRelatedID = 0;
    float fFloat = 0.0;
    double fDouble = 0.0;
    int nDecimal = 0;
    StDate date;
    StTime time;
    StDateTime datetime;
    StPoint<int> point;
    StRect<int> rect;
}StCourse;



#endif // DATA_TYPE_DEFINATION_H
