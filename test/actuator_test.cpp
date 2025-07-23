/**
 * @brief Test the actuator concept.
 *
 * @file actuator_test.cpp
 * @author Nicu Popescu
 * @date 2021
 */
#include <actuator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace untangle {
namespace test {

class shape
{
public:
  virtual ~shape(){}
  virtual void rotate(int angle) const = 0;
};


class triangle : public shape
{
public:
  void rotate(int angle) const override
  {
    std::cout << "triangle::rotate " << angle << std::endl;
  }

  void height_in(int h) { std::cout << "triangle::height_in" << std::endl; height = h; }
  int height_out() { std::cout << "triangle::height_out" << std::endl; return height; }

  void test_vr() { std::cout << "triangle::test_vr" << std::endl; }
  void test_vr_args(int x, int y) { std::cout << "triangle::test_vr_args " << x << ", "<< y <<std::endl; }

private:
  int height;
};

class triangle_mock : public shape {
public:
  MOCK_CONST_METHOD1(rotate, void(int));
};

class circle : public shape
{
public:
  void rotate(int angle) const override { std::cout << "circle::rotate " << angle << std::endl; }

  void height_in(int h) { std::cout << "circle::height_in" << std::endl; height = h; }
  int height_out() { std::cout << "circle::height_out" << std::endl; return height; }

  void test_vr() { std::cout << "circle::test_vr" << std::endl; }
  void test_vr_args(int x, int y) { std::cout << "circle::test_vr_args " << x << ", "<< y <<std::endl; }

private:
  int height;
};

class circle_mock : public shape {
public:
  MOCK_CONST_METHOD1(rotate, void(int));
};

class square : public shape
{
public:
  void rotate(int angle) const override { std::cout << "square::rotate " << angle << std::endl; }

  void height_in(int h) { std::cout << "square::height_in" << std::endl; height = h; }
  int height_out() { std::cout << "square::height_out" << std::endl; return height; }

  void test_vr() { std::cout << "square::test_vr" << std::endl; }
  void test_vr_args(int x, int y) { std::cout << "square::test_vr_args " << x << ", "<< y <<std::endl; }

private:
  int height;
};

class square_mock : public shape {
public:
  MOCK_CONST_METHOD1(rotate, void(int));
};

void rotate(int angle) { std::cout << "function::rotate " << angle << std::endl; }

//! [test_polymorphism1]
void rotate_shapes(const std::vector<shape*>& shapes, int angle)
{
  for (const auto& s : shapes)
  {
    s->rotate(angle);
  }
}


// void test_extract_results()
// {
//   //! [test_extract_results]
//   std::shared_ptr<triangle> t(new triangle);
//   std::shared_ptr<circle> c(new circle);
//   std::shared_ptr<square> s(new square);

//   auto action1 = untangle::bind(t, &triangle::height_in);
//   auto action2 = untangle::bind(c, &circle::height_in);
//   auto action3 = untangle::bind(s, &square::height_in);

//   auto actuator_height_in = untangle::connect(action1, action2, action3);
//   actuator_height_in(80);

//   auto action4 = untangle::bind(t, &triangle::height_out);
//   auto action5 = untangle::bind(c, &circle::height_out);
//   auto action6 = untangle::bind(s, &square::height_out);

//   auto actuator_height_out = untangle::connect(action4, action5, action6);
//   actuator_height_out();

//   std::cout << "\nextract result\n" << std::endl;

//   for (const auto r : actuator_height_out.results)
//   {
//     std::cout << r << " ";
//   }
//   //! [test_extract_results]
// }

// void test_void_return()
// {
//   //! [test_void_return]
//   std::shared_ptr<triangle> t(new triangle);
//   std::shared_ptr<circle> c(new circle);
//   std::shared_ptr<square> s(new square);

//   auto action1 = untangle::bind(t, &triangle::test_vr);
//   auto action2 = untangle::bind(c, &circle::test_vr);
//   auto action3 = untangle::bind(s, &square::test_vr);

//   auto actuator_vr = untangle::connect(action1, action2, action3);

//   std::cout << "\nvoid return\n" << std::endl;
//   actuator_vr();
//   //! [test_void_return]
// }

// void test_void_return_and_args()
// {
//   //! [test_void_return_and_args]
//   std::shared_ptr<triangle> t(new triangle);
//   std::shared_ptr<circle> c(new circle);
//   std::shared_ptr<square> s(new square);

//   auto action1 = untangle::bind(t, &triangle::test_vr_args);
//   auto action2 = untangle::bind(c, &circle::test_vr_args);
//   auto action3 = untangle::bind(s, &square::test_vr_args);

//   auto actuator_vr_args = untangle::connect(action1, action2, action3);

//   std::cout << "\nvoid return adn arguments\n" << std::endl;
//   actuator_vr_args(90, 100);
//   //! [test_void_return_and_args]
// }

// void test_polymorphism_named_actions()
// {
//   //! [test_polymorphism_named_actions2]
//   // How to use actuator instead of polymorphism.

//   std::shared_ptr<triangle> t(new triangle);
//   std::shared_ptr<circle> c(new circle);
//   std::shared_ptr<square> s(new square);

//   // using polymorphism
//   std::vector<shape*> shapes;
//   shapes.push_back(t.get());
//   shapes.push_back(c.get());
//   shapes.push_back(s.get());

//   std::cout << "using polymorphism\n" << std::endl;
//   rotate_shapes(shapes, 10);

//   // using actuator
//   auto action1 = untangle::bind(t, &triangle::rotate);
//   auto action2 = untangle::bind(c, &circle::rotate);
//   auto action3 = untangle::bind(s, &square::rotate);

//   auto actuator_rotate = untangle::connect(std::make_pair("triangle", &action1),
//                                            std::make_pair("circle", &action2),
//                                            std::make_pair("square", &action3));
//   actuator_rotate.remove("circle");
//   actuator_rotate.add("circle", &action2);
//   std::cout << "\nusing named actuator\n" << std::endl;
//   auto hasCircle = actuator_rotate.has_action("circle");
//   std::cout << "has circle:" << hasCircle << std::endl;
//   actuator_rotate.invokeAction("circle", 20);

//   t.reset();
//   actuator_rotate.invokeAction("triangle", 20);
//   //! [test_polymorphism_named_actions2]
// }

TEST(test_actuator, test_polymorphism_using_shared_pointers) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(t.get());
  shapes.push_back(c.get());
  shapes.push_back(s.get());

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  rotate_shapes(shapes, 10);
  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());

  // using actuator
  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_polymorphism_using_pointers) {
  triangle_mock t;
  circle_mock c;
  square_mock s;

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(&t);
  shapes.push_back(&c);
  shapes.push_back(&s);

  EXPECT_CALL(t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(s, rotate(testing::_)).WillOnce(testing::Return());
  rotate_shapes(shapes, 10);

  testing::Mock::VerifyAndClearExpectations(&t);
  testing::Mock::VerifyAndClearExpectations(&c);
  testing::Mock::VerifyAndClearExpectations(&s);

  // using actuator
  EXPECT_CALL(t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(&t, &triangle_mock::rotate);
  auto action2 = untangle::bind(&c, &circle_mock::rotate);
  auto action3 = untangle::bind(&s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(&t);
  testing::Mock::VerifyAndClearExpectations(&c);
  testing::Mock::VerifyAndClearExpectations(&s);
}

TEST(test_actuator, test_assignment) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  untangle::actuator<decltype(actuator_rotate.type())> actuator_rotate_1;
  actuator_rotate_1 = actuator_rotate;
  actuator_rotate_1(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_add) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  EXPECT_CALL(*t, rotate(testing::_)).Times(2).WillRepeatedly(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate.add(&action1);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_remove) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  EXPECT_CALL(*t, rotate(testing::_)).Times(0);
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate.remove(&action1);
  actuator_rotate(50);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_remove_by_empty_action) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(70);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  std::function<void(int)> action_empty;
  actuator_rotate = untangle::connect(action1, action_empty, action3);
  actuator_rotate(80);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_invalid_action) {
  std::shared_ptr<triangle_mock> t{new triangle_mock};
  std::shared_ptr<circle_mock> c{new circle_mock};
  std::shared_ptr<square_mock> s{new square_mock};

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  c.reset();
  actuator_rotate(60);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

} // namespace test
} // namespace untangle