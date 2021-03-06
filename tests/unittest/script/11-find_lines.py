def unittest(data_path, temp_path):
    import image
    img = image.Image("/sd/unittest/data/shapes.ppm", copy_to_fb=True)
    lines = img.find_lines(threshold = 10000, theta_margin = 25, rho_margin = 25)
    return len(lines) == 4 and\
    lines[0][0:] == (22, 0, 22, 119, 119, 17340, 0, 22)  and\
    lines[1][0:] == (0, 39, 159, 39, 159, 17340, 90, 39) and\
    lines[2][0:] == (57, 0, 57, 119, 119, 17340, 0, 57)  and\
    lines[3][0:] == (0, 75, 159, 75, 159, 21420, 90, 75)
