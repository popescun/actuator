/**
 * @brief Test the actuator concept.
 *
 * @file main.cpp
 * @author Nicu Popescu
 * @date 2018 - 2021
 */
#include <actuator.h>

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

  // actuators
  untangle::actuator<std::function<void(int)>> actuator_rotate;

private:
  int height;
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

void rotate(int angle) { std::cout << "function::rotate " << angle << std::endl; }

//! [test_polymorphism1]
void rotate_shapes(std::vector<shape*>& shapes, int angle)
{
  for (const auto& s : shapes)
  {
    s->rotate(angle);
  }
}
//! [test_polymorphism1]

void test_polymorphism()
{
  //! [test_polymorphism2]
  // How to use actuator instead of polymorphism.

  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(t.get());
  shapes.push_back(c.get());
  shapes.push_back(s.get());

  std::cout << "using polymorphism\n" << std::endl;
  rotate_shapes(shapes, 10);

  // using actuator
  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  std::cout << "\nusing actuator\n" << std::endl;
  actuator_rotate(20);
  //! [test_polymorphism2]
}

void test_assignment()
{
  //! [test_assignment]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);

  std::cout << "assign to an actuator member of class triangle\n" << std::endl;
  t->actuator_rotate = actuator_rotate;
  t->actuator_rotate(30);
  //! [test_assignment]
}

void test_add()
{
  //! [test_add]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);

  std::cout << "\nadd an action\n" << std::endl;
  actuator_rotate.add(&action1);
  actuator_rotate(40);
  //! [test_add]
}

void test_remove()
{
  //! [test_remove]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);

  std::cout << "\nremove an action\n" << std::endl;
  actuator_rotate.remove(&action1);
  actuator_rotate(50);
  //! [test_remove]
}

void test_invalid_action()
{
  //! [test_invalid_action]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);

  std::cout << "\nthe bound object is reset: the action is not executed and removed \n" << std::endl;
  c.reset();
  actuator_rotate(60);
  //! [test_invalid_action]
}

void test_remove_by_empty_action()
{
  //! [test_remove_by_empty_action]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);

  std::cout << "\nthe action is removed when an empty action is connected in its place\n" << std::endl;
  std::function<void(int)> action_empty;
  actuator_rotate = untangle::connect(action1, action_empty, action3);
  actuator_rotate(70);
  //! [test_remove_by_empty_action]
}

void test_extract_results()
{
  //! [test_extract_results]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::height_in);
  auto action2 = untangle::bind(c, &circle::height_in);
  auto action3 = untangle::bind(s, &square::height_in);

  auto actuator_height_in = untangle::connect(action1, action2, action3);
  actuator_height_in(80);

  auto action4 = untangle::bind(t, &triangle::height_out);
  auto action5 = untangle::bind(c, &circle::height_out);
  auto action6 = untangle::bind(s, &square::height_out);

  auto actuator_height_out = untangle::connect(action4, action5, action6);
  actuator_height_out();

  std::cout << "\nextract result\n" << std::endl;

  for (const auto r : actuator_height_out.results)
  {
    std::cout << r << " ";
  }
  //! [test_extract_results]
}

void test_void_return()
{
  //! [test_void_return]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::test_vr);
  auto action2 = untangle::bind(c, &circle::test_vr);
  auto action3 = untangle::bind(s, &square::test_vr);

  auto actuator_vr = untangle::connect(action1, action2, action3);

  std::cout << "\nvoid return\n" << std::endl;
  actuator_vr();
  //! [test_void_return]
}

void test_void_return_and_args()
{
  //! [test_void_return_and_args]
  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  auto action1 = untangle::bind(t, &triangle::test_vr_args);
  auto action2 = untangle::bind(c, &circle::test_vr_args);
  auto action3 = untangle::bind(s, &square::test_vr_args);

  auto actuator_vr_args = untangle::connect(action1, action2, action3);

  std::cout << "\nvoid return adn arguments\n" << std::endl;
  actuator_vr_args(90, 100);
  //! [test_void_return_and_args]
}

void test_polymorphism_named_actions()
{
  //! [test_polymorphism_named_actions2]
  // How to use actuator instead of polymorphism.

  std::shared_ptr<triangle> t(new triangle);
  std::shared_ptr<circle> c(new circle);
  std::shared_ptr<square> s(new square);

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(t.get());
  shapes.push_back(c.get());
  shapes.push_back(s.get());

  std::cout << "using polymorphism\n" << std::endl;
  rotate_shapes(shapes, 10);

  // using actuator
  auto action1 = untangle::bind(t, &triangle::rotate);
  auto action2 = untangle::bind(c, &circle::rotate);
  auto action3 = untangle::bind(s, &square::rotate);

  auto actuator_rotate = untangle::connect(std::make_pair("triangle", &action1),
                                           std::make_pair("circle", &action2),
                                           std::make_pair("square", &action3));
  actuator_rotate.remove("circle");
  actuator_rotate.add("circle", &action2);
  std::cout << "\nusing named actuator\n" << std::endl;
  auto hasCircle = actuator_rotate.has_action("circle");
  std::cout << "has circle:" << hasCircle << std::endl;
  actuator_rotate.invokeAction("circle", 20);

  t.reset();
  actuator_rotate.invokeAction("triangle", 20);
  //! [test_polymorphism_named_actions2]
}

int main()
{
  test_polymorphism();
  test_assignment();
  test_add();
  test_remove();
  test_invalid_action();
  test_remove_by_empty_action();
  test_extract_results();
  test_void_return();
  test_void_return_and_args();
  // test named actions
  test_polymorphism_named_actions();

  return 0;
}