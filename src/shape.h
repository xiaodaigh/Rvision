Rcpp::List _findContours(Image image, int mode, int method, Rcpp::NumericVector offset) {
  std::vector< std::vector< cv::Point > > contours;
  std::vector< cv::Vec4i > hierarchy;

  cv::findContours(image.image, contours, hierarchy, mode, method, cv::Point(offset(0), offset(1)));

  int size = 0;
  for (uint i = 0; i < contours.size(); i++) {
    size += contours[i].size();
  }

  Rcpp::NumericMatrix contours_mat(size, 3);
  colnames(contours_mat) = Rcpp::CharacterVector::create("id", "x", "y");
  Rcpp::NumericMatrix hierarchy_mat(contours.size(), 5);
  colnames(hierarchy_mat) = Rcpp::CharacterVector::create("id", "after", "before", "child", "parent");

  int counter = 0;
  for (uint i = 0; i < contours.size(); i++) {
    for (uint j = 0; j < contours[i].size(); j++) {
      contours_mat(counter, 0) = i;
      contours_mat(counter, 1) = contours[i][j].x;
      contours_mat(counter, 2) = contours[i][j].y;
      counter += 1;
    }

    hierarchy_mat(i, 0) = i;
    hierarchy_mat(i, 1) = hierarchy[i][0];
    hierarchy_mat(i, 2) = hierarchy[i][1];
    hierarchy_mat(i, 3) = hierarchy[i][2];
    hierarchy_mat(i, 4) = hierarchy[i][3];
  }

  return Rcpp::List::create(Rcpp::Named("contours") = contours_mat,
                            Rcpp::Named("hierarchy") = hierarchy_mat);
}

double _contourArea(Rcpp::NumericVector x, Rcpp::NumericVector y, bool oriented) {
  std::vector<cv::Point> poly;

  for (uint i = 0; i < x.size(); i++) {
    poly.push_back(cv::Point2f(x(i), y(i)));
  }

  return cv::contourArea(poly, oriented);
}

Rcpp::List _connectedComponents(Image image, int connectivity, int algorithm) {
  cv::Mat labels;
  int n = cv::connectedComponents(image.image, labels, connectivity, CV_16U, algorithm);

  std::vector<cv::Point> locs;
  cv::findNonZero(labels > 0, locs);

  Rcpp::NumericMatrix table_mat(locs.size(), 3);
  colnames(table_mat) = Rcpp::CharacterVector::create("id", "x", "y");

  for (uint i = 0; i < locs.size(); i++) {
    table_mat(i, 0) = labels.at< uint16_t >(locs[i]);
    table_mat(i, 1) = locs[i].x + 1;
    table_mat(i, 2) = -locs[i].y + image.image.rows;
  }

  return Rcpp::List::create(Rcpp::Named("n") = n - 1,
                            Rcpp::Named("labels") = Image(labels),
                            Rcpp::Named("table") = table_mat);
}

void _watershed(Image image, Image markers) {
  cv::watershed(image.image, markers.image);
}

Rcpp::List _fitEllipse(arma::fmat points) {
  cv::Mat cvpoints;
  arma2cv(points, cvpoints);
  cv::RotatedRect box;
  box = cv::fitEllipse(cvpoints);

  return Rcpp::List::create(Rcpp::Named("angle") = box.angle,
                            Rcpp::Named("height") = box.size.height,
                            Rcpp::Named("width") = box.size.width,
                            Rcpp::Named("center") = Rcpp::NumericVector::create(box.center.x, box.center.y));
}

Rcpp::List _fitEllipseAMS(arma::fmat points) {
  cv::Mat cvpoints;
  arma2cv(points, cvpoints);
  cv::RotatedRect box;
  box = cv::fitEllipseAMS(cvpoints);

  return Rcpp::List::create(Rcpp::Named("angle") = box.angle,
                            Rcpp::Named("height") = box.size.height,
                            Rcpp::Named("width") = box.size.width,
                            Rcpp::Named("center") = Rcpp::NumericVector::create(box.center.x, box.center.y));
}

Rcpp::List _fitEllipseDirect(arma::fmat points) {
  cv::Mat cvpoints;
  arma2cv(points, cvpoints);
  cv::RotatedRect box;
  box = cv::fitEllipseDirect(cvpoints);

  return Rcpp::List::create(Rcpp::Named("angle") = box.angle,
                            Rcpp::Named("height") = box.size.height,
                            Rcpp::Named("width") = box.size.width,
                            Rcpp::Named("center") = Rcpp::NumericVector::create(box.center.x, box.center.y));
}

std::vector< int > _convexHull(arma::fmat points, bool clockwise) {
  cv::Mat cvpoints;
  arma2cv(points, cvpoints);
  std::vector< int > out;
  cv::convexHull(cvpoints, out, clockwise, false);
  return out;
}

Rcpp::IntegerMatrix _convexityDefects(Rcpp::NumericMatrix contour, std::vector< int > convexhull) {
  std::vector< cv::Point > contourpoints(contour.nrow());
  std::vector< cv::Vec4i > defects;

  for (int i = 0; i < contour.nrow(); i++) {
    contourpoints[i].x = contour(i, 1);
    contourpoints[i].y = contour(i, 2);
  }

  cv::convexityDefects(contourpoints, convexhull, defects);

  Rcpp::IntegerVector start_index(defects.size());
  Rcpp::IntegerVector end_index(defects.size());
  Rcpp::IntegerVector farthest_pt_index(defects.size());
  Rcpp::IntegerVector fixpt_depth(defects.size());

  Rcpp::IntegerMatrix defects_mat(defects.size(), 4);
  colnames(defects_mat) = Rcpp::CharacterVector::create("start_index", "end_index", "farthest_pt_index", "fixpt_depth");

  for (uint i = 0; i < defects.size(); i++) {
    defects_mat(i, 0) = defects[i][0];
    defects_mat(i, 1) = defects[i][2];
    defects_mat(i, 2) = defects[i][2];
    defects_mat(i, 3) = defects[i][3];
  }

  return defects_mat;
}

Rcpp::NumericVector _moments(Rcpp::NumericMatrix contour) {
  std::vector< cv::Point > contourpoints(contour.nrow());

  for (int i = 0; i < contour.nrow(); i++) {
    contourpoints[i].x = contour(i, 1);
    contourpoints[i].y = contour(i, 2);
  }

  cv::Moments m = cv::moments(contourpoints);

  Rcpp::NumericVector out = { m.m00, m.m10, m.m01, m.m20, m.m11, m.m02, m.m30,
    m.m21, m.m12, m.m03, m.mu20, m.mu11, m.mu02, m.mu30, m.mu21, m.mu12, m.mu03,
    m.nu20, m.nu11, m.nu02, m.nu30, m.nu21, m.nu12, m.nu03 };

  return out;
}

Rcpp::List _minAreaRect(arma::fmat points) {
  cv::Mat cvpoints;
  arma2cv(points, cvpoints);
  cv::RotatedRect box;
  box = minAreaRect(cvpoints);

  return Rcpp::List::create(Rcpp::Named("angle") = box.angle,
                            Rcpp::Named("height") = box.size.height,
                            Rcpp::Named("width") = box.size.width,
                            Rcpp::Named("center") = Rcpp::NumericVector::create(box.center.x, box.center.y));
}